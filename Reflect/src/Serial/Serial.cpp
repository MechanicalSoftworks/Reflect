#include "Core/Serial.h"
#include "Core/Allocator.h"
#include "Core/FieldIO.h"
#include <filesystem>

struct header_t
{
	static constexpr uint8_t VERSION = 1;
	static constexpr uint8_t PAD = 0;
	static constexpr uint16_t ENDIAN = 1;

	header_t(Reflect::Serialiser& s, std::ostream& fout)
	{
		Reflect::FieldImpl::write(s, fout, VERSION);
		Reflect::FieldImpl::write(s, fout, PAD);
		Reflect::FieldImpl::write(s, fout, ENDIAN);
	}

	header_t(Reflect::Unserialiser &u, std::istream& fin)
	{
		uint8_t version, pad;
		uint16_t endian;

		Reflect::FieldImpl::read(u, fin, version);
		Reflect::FieldImpl::read(u, fin, pad);
		Reflect::FieldImpl::read(u, fin, endian);

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
	void Serialiser::Write(std::ostream& fout, const IReflect& root)
	{
		// Get a temp file to store the binary data.
		char temp_path[512];
		tmpnam(temp_path);

		// Open the temp file the entity data is written to.
		std::ofstream oftemp(temp_path, std::ios::out | std::ios::binary);
		if (!oftemp.good())
		{
			throw std::runtime_error("Couldn't create temp Serialiser file");
		}

		// Automatically delete the file when the method exits.
		// RAII will take care of this in the good, and bad (exception!) cases.
		DeleteFile df(temp_path);

		// Write the entity data to the temp file.
		// Schema and string pool information will be written at the same time.
		FieldImpl::write(*this, oftemp, m_string_pool.Add(root.GetClass()->GetName()));
		WriteField(*this, oftemp, root);
		oftemp.close();

		// Write the header, now that's been populated by object serialisation.
		header_t hdr(*this, fout);
		WriteField(*this, fout, m_string_pool);
		WriteField(*this, fout, m_schemas);
		WriteField(*this, fout, m_user_data_types);
		
		// Copy the binary data into the output stream.
		std::ifstream iftemp(temp_path, std::ios::in | std::ios::binary);
		fout << iftemp.rdbuf();
	}

	void Serialiser::AddSchema(const Reflect::Class& static_class)
	{
		for (const auto* it = &static_class; it; it = it->GetSuperClass())
		{
			const std::string class_name = it->GetName();

			if (m_schemas.find(class_name) != m_schemas.end())
			{
				return;
			}

			m_schemas.insert(std::pair(class_name, FieldSchema(it, m_string_pool)));
		}
	}

	void Serialiser::AddUserDataType(const std::string& name, size_t sz)
	{
		m_string_pool.Add(name);
		m_user_data_types.insert(std::pair(name, UserDataType(name, sz)));
	}

	//==========================================================================
	//==========================================================================

	bool Unserialiser::ParseHeader(std::istream& fin)
	{
		// Read the header.
		header_t header(*this, fin);

		// Read the objects.
		ReadField<decltype(m_string_pool), 0>(*this, fin, &m_string_pool);
		ReadField<decltype(m_schemas), 0>(*this, fin, &m_schemas);
		ReadField<decltype(m_user_data_types), 0>(*this, fin, &m_user_data_types);

		bool error = false;

		for (const auto& message_schema : m_schemas)
		{
			const auto* current_schema = Class::Lookup(message_schema.first);

			// Look for entities that don't exist anymore.
			if (!current_schema)
			{
				m_schema_differences.push_back(SchemaDifference(true, std::string("Class ") + message_schema.first + " doesn't exist anymore"));
				error = true;
				continue;
			}

			const auto current_fields = current_schema->GetMembers({ "serialise"}, false);
			const auto& message_fields = message_schema.second.fields;

			for (const auto& message_field : message_fields)
			{
				const auto& current_field = std::find_if(current_fields.begin(), current_fields.end(),
					[&message_field](const auto& f)
					{
						return f.GetName() == message_field.name;
					});

				// Look for fields that are in the message, but removed from the object.
				if (current_field == current_fields.end())
				{
					m_schema_differences.push_back(SchemaDifference(false, std::string("Field ") + message_schema.first + "::" + message_field.name + " no longer exists. It will be ignored"));
					continue;
				}

				// Look for a change in data type.
				if (current_field->GetTypeName() != message_field.type)
				{
					m_schema_differences.push_back(SchemaDifference(false, std::string("Field ") + message_schema.first + "::" + message_field.name + " was a '" + message_field.type + "', but now is a '" + current_field->GetTypeName() + "'. It will be left at its default value"));
					continue;
				}
			}

			for (const auto& current_field : current_fields)
			{
				const auto& message_field = std::find_if(message_fields.begin(), message_fields.end(),
					[&current_field](const auto& f)
					{
						return f.name == current_field.GetName();
					});

				// Look for fields that exist in the object, but aren't included in the message.
				if (message_field == message_fields.end())
				{
					m_schema_differences.push_back(SchemaDifference(false, std::string("Field ") + message_schema.first + "::" + current_field.GetName() + " was added. It will be left at its default value"));
					continue;
				}
			}
		}

		return error;
	}

	void Unserialiser::Read(std::istream& fin, IReflect* object, IReflect* outer)
	{
		// Read the root object type.
		StringPool::index_t root_type_index;
		FieldImpl::read(*this, fin, root_type_index);

		// Get the root object class.
		const auto root_type_string = m_string_pool.At(root_type_index);
		m_root_class = Class::Lookup(root_type_string);
		if (!m_root_class)
		{
			throw std::runtime_error("Failed to find the message root object type");
		}

		// Create the root entity.
		if (!object)
		{
			m_root = std::move(Allocator::Create<IReflect>(m_root_class, outer));
			object = m_root.get();
		}
		else
		{
			const Initialiser init(m_root_class, outer);
			m_root_class->Constructor(object, init);
		}

		// Recreate the scene.
		PushCurrentObject(object);
		ReadField<IReflect, 0>(*this, fin, object);
		PopCurrentObject();
	}

	const FieldSchema& Unserialiser::GetSchema(const std::string& name) const
	{
		const auto it = m_schemas.find(name);
		if (it != m_schemas.end())
		{
			return it->second;
		}

		throw std::runtime_error("Unknown schema '" + name + "'");
	}

	const UserDataType* Unserialiser::GetUserDataType(const std::string& name) const
	{
		const auto it = m_user_data_types.find(name);
		if (it != m_user_data_types.end())
		{
			return &it->second;
		}

		return nullptr;
	}

	void Unserialiser::RegisterSchemaAlias(const char* alias, const char* old_type)
	{
		m_schemas[alias] = m_schemas[old_type];
	}

	//--------------------------------------------------------------------------
	// StringPool implementation.
	//--------------------------------------------------------------------------

	StringPool::entry_t::entry_t(Unserialiser& u, std::istream& fin)
	{
		FieldImpl::read(u, fin, offset);
		FieldImpl::read(u, fin, length);
	}

	void StringPool::entry_t::operator()(Serialiser& s, std::ostream& fout) const
	{
		FieldImpl::write(s, fout, offset);
		FieldImpl::write(s, fout, length);
	}

	StringPool::index_t StringPool::Add(const std::string_view& view)
	{
		const std::string s(view);
		const auto it = m_cache.find(std::string(s));
		if (it != m_cache.end())
		{
			return it->second;
		}

		m_cache[s] = (index_t)m_entries.size();
		m_entries.push_back(entry_t((uint32_t)m_pool.length(), (uint32_t)view.length()));
		m_pool.append(view);

		return (index_t)m_entries.size() - 1;
	}

	const std::string_view StringPool::At(index_t i) const
	{
		if (i < 0 || i >= m_entries.size())
		{
			throw std::out_of_range("String pool index out of range");
		}

		const auto& e = m_entries.at(i);
		return std::string_view(m_pool.c_str() + e.offset, e.length);
	}

	void StringPool::Serialise(Serialiser& s, std::ostream& fout) const
	{
		FieldImpl::write(s, fout, (uint32_t)m_pool.length());
		fout.write(m_pool.c_str(), m_pool.length());

		FieldImpl::write(s, fout, (uint32_t)m_entries.size());
		for (index_t i = 0; i < m_entries.size(); i++)
		{
			m_entries[i](s, fout);
		}
	}

	void StringPool::Unserialise(Unserialiser& u, std::istream& fin)
	{
		index_t count, length;
		char buf[256];

		// Reserve memory for the string pool.
		// Means malloc cost is minimized - just need to optimise for input.
		// Don't use resize()! That will initialise each character!
		FieldImpl::read(u, fin, length);
		m_pool.reserve(length);

		// Read the string in 256 char chunks.
		while (length)
		{
			index_t read_sz = std::min(length, (index_t)sizeof(buf));
			fin.read(buf, read_sz);
			m_pool.append(buf, read_sz);
			length -= read_sz;
		}

		// Read the pool metadata.
		FieldImpl::read(u, fin, count);
		m_entries.reserve(count);
		for (index_t i = 0; i < count; i++)
		{
			m_entries.push_back(entry_t(u, fin));
		}
	}
}