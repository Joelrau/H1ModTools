#include "filesystem.hpp"

namespace utils
{
	namespace filesystem
	{
		file::file(const std::string& filepath_)
		{
			this->initialize(filepath_);
		}

		void file::initialize(const std::filesystem::path& filepath_)
		{
			if (filepath_.empty())
			{
				return;
			}

			this->filepath = filepath_;
			this->parent_path = this->filepath.parent_path().string();
			this->filename = this->filepath.filename().string();
		}

		file::file()
		{
			this->fp = nullptr;
		}

		file::file(const file& other) : file(other.filepath.generic_string())
		{
		}

		file::file(file&& other) noexcept
		{
			this->initialize(other.filepath);
			this->fp = other.fp;
			other.fp = nullptr;
		}

		file& file::operator=(const file& other)
		{
			if (&other != this)
			{
				this->close();
				this->initialize(other.filepath);
			}

			return *this;
		}

		file& file::operator=(file&& other) noexcept
		{
			if (&other != this)
			{
				this->close();
				this->initialize(other.filepath);
				this->fp = other.fp;
				other.fp = nullptr;
			}

			return *this;
		}

		file::~file()
		{
			this->close();
		}

		FILE* file::get_fp()
		{
			return this->fp;
		}

		bool file::exists(bool use_path)
		{
			this->open("rb", use_path);
			if (this->fp)
			{
				this->close();
				return true;
			}
			return false;
		}

		bool file::exists()
		{
			return this->exists(true);
		}

		errno_t file::open(std::string mode, bool use_path)
		{
			if (mode[0] == 'r')
            {
                return fopen_s(&this->fp, this->filepath.string().data(), mode.data());
            }
            if (mode[0] == 'w' || mode[0] == 'a')
            {
                auto dir = this->filepath.string() + this->parent_path;
                create_directory(dir);
                return fopen_s(&this->fp, this->filepath.string().data(), mode.data());
            }

			return fopen_s(&this->fp, this->filepath.string().data(), mode.data());
		}

		size_t file::write_string(const std::string& str)
		{
			if (this->fp)
			{
				return fwrite(str.data(), str.size() + 1, 1, this->fp);
			}
			return 0;
		}

		size_t file::write_string(const char* str)
		{
			return this->write_string(std::string(str));
		}

		size_t file::write(const void* buffer, size_t size, size_t count)
		{
			if (this->fp)
			{
				return fwrite(buffer, size, count, this->fp);
			}
			return 0;
		}

		size_t file::write(const std::string& str)
		{
			return this->write(str.data(), str.size(), 1);
		}

		int file::seek(size_t offset, int origin)
		{
			return _fseeki64(this->fp, offset, origin);
		}

		size_t file::tell()
		{
			return _ftelli64(this->fp);
		}

		inline size_t get_string_size(FILE* fp)
		{
			auto i = _ftelli64(fp);
			char c;
			size_t size = 0;
			while (fread(&c, sizeof(char), 1, fp) == 1)
			{
				if (c == '\0') // null terminator
				{
					break;
				}
				size++;
			}
			_fseeki64(fp, i, SEEK_SET);
			return size;
		}

		size_t file::read_string(std::string* str)
		{
			if (this->fp)
			{
				auto size = get_string_size(this->fp);
				str->resize(size);
				fread(str->data(), sizeof(char), size + 1, this->fp);
			}
			return 0;
		}

		size_t file::read(void* buffer, size_t size, size_t count)
		{
			if (this->fp)
			{
				return fread(buffer, size, count, this->fp);
			}
			return 0;
		}

		int file::close()
		{
			if (this->fp)
			{
				return fclose(this->fp);
			}
			return -1;
		}

		bool file::create_path()
		{
			return create_directory(this->parent_path);
		}

		std::size_t file::size()
		{
			if (this->fp)
			{
				auto i = _ftelli64(this->fp);
				_fseeki64(this->fp, 0, SEEK_END);

				auto ret = _ftelli64(this->fp);
				_fseeki64(this->fp, i, SEEK_SET);

				return ret;
			}

			return 0;
		}

		std::vector<std::uint8_t> file::read_bytes(std::size_t size)
		{
			if (this->fp && size)
			{
				// alloc vector
				std::vector<std::uint8_t> buffer;
				buffer.resize(size);

				// read data
				fread(&buffer[0], size, 1, this->fp);

				// return data
				return buffer;
			}

			return {};
		}

		bool create_directory(const std::string& name)
		{
			if (!name.empty())
			{
				return std::filesystem::create_directories(name);
			}
			return false;
		}
	}
}