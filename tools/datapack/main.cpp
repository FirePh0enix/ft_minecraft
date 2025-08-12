#include "DataPack.hpp"

#include <iostream>

/*
    ./datapacktool <output> <prefix_to_remove> [files...]
 */

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "not enough arguments\n";
        return 1;
    }

    std::string output_file = argv[1];
    std::string prefix_to_remove = argv[2];
    int files = argc - 3;

    DataPack data;
    data.open(output_file);

    for (int i = 0; i < files; i++)
    {
        std::string arg = argv[i + 3];
        std::ifstream ifs(arg, std::ifstream::ate);

        if (!ifs.is_open())
        {
            std::cerr << "Cannot open file `" << arg << "`\n";
            continue;
        }

        std::ifstream::pos_type size = ifs.tellg();
        ifs.seekg(0);

        std::vector<char> buffer(size);
        ifs.read(buffer.data(), size);

        data.add_file_to_data_pack(arg.substr(prefix_to_remove.size()), buffer);
        std::cerr << "Adding file `" << arg << "` to " << output_file << "\n";
    }
}
