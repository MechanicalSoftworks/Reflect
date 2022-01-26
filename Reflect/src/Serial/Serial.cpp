#include "Core/Serial.h"
#include <filesystem>

struct header_t
{
	static constexpr uint8_t VERSION = 1;
	static constexpr uint8_t PAD = 0;
	static constexpr uint16_t ENDIAN = 1;

	header_t(std::ostream& fout)
	{
		fout << VERSION << PAD << ENDIAN;
	}

	header_t(std::istream& fin)
	{
		uint8_t version, pad;
		uint16_t endian;

		fin >> version >> pad >> endian;

		if (version != VERSION)
		{
			throw std::runtime_error("Old message format");
		}

		if (pad != PAD)
		{
			throw std::runtime_error("Corrupt message");
		}

		if (endian != ENDIAN)
		{
			throw std::runtime_error("Unsupported endianness");
		}
	}
};

class DeleteFile
{
public:
	DeleteFile(const std::string& path) : m_path(path) {}
	~DeleteFile()
	{
		if (std::filesystem::exists(m_path))
		{
			std::filesystem::remove(m_path);
		}
	}

private:
	const std::string m_path;
};

namespace Reflect
{
	Serialiser::Serialiser(std::ostream& fout, const IReflect& root)
	{
		// Get a temp file to store the binary data.
		char temp_path[512];
		tmpnam(temp_path);

		// Open the temp file the entity data is written to.
		std::fstream ftemp(temp_path, std::ios::binary);
		if (!ftemp.good())
		{
			throw std::runtime_error("Couldn't create temp Serialiser file");
		}

		// Automatically delete the file when the method exits.
		// RAII will take care of this in the good, and bad (exception!) cases.
		DeleteFile df(temp_path);

		// Write the entity data to the temp file.
		// Schema and string pool information will be written at the same time.
		ftemp << m_string_pool.Add(root.GetClass()->GetName());
		root.Serialise(*this, ftemp);

		// Write the header, now that's been populated by object serialisation.
		header_t hdr(fout);
		FieldImpl::write(*this, fout, m_string_pool);
		FieldImpl::write(*this, fout, m_schemas);
		
		// Copy the binary data into the output stream.
		fout << ftemp.rdbuf();
	}

	void Serialiser::AddSchema(const Reflect::Class& static_class)
	{
		const std::string class_name = static_class.GetName();

		if (m_schemas.find(class_name) != m_schemas.end())
		{
			return;
		}

		m_schemas.insert(std::pair(class_name, FieldSchema(&static_class, m_string_pool)));
	}

	//==========================================================================
	//==========================================================================

	Unserialiser::Unserialiser(std::istream& fin, AlignedAlloc alloc, AlignedFree free)
		: m_alloc(alloc)
		, m_free(free)
	{
		// Read the header.
		header_t header(fin);

		// Read the objects.
		FieldImpl::read(*this, fin, m_string_pool);
		FieldImpl::read(*this, fin, m_schemas);

		// Read the root object type.
		StringPool::index_t root_type_index;
		fin >> root_type_index;

		// Get the root object class.
		const auto root_type_string = m_string_pool.At(root_type_index);
		m_root_class = Class::Lookup(root_type_string);
		if (!m_root_class)
		{
			throw std::runtime_error("Failed to find the message root object type");
		}

		// Create the root entity.
		m_root = (IReflect*)alloc(m_root_class->GetRawSize(), m_root_class->GetAlignment());
		if (!m_root)
		{
			throw std::bad_alloc();
		}
		m_root_class->Constructor(m_root);

		// Recreate the scene.
		m_root->Unserialise(*this, fin);
	}

	Unserialiser::~Unserialiser()
	{
		if (m_root)
		{
			m_root_class->Destructor(m_root);
			m_free(m_root);
		}
	}

	IReflect* Unserialiser::Detach()
	{
		IReflect* t = m_root;
		m_root = nullptr;
		m_root_class = nullptr;
		return t;
	}
}