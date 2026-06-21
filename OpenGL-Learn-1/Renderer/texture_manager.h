#pragma once
#include <string>
#include <unordered_map>

class texture_manager
{
public:

    bool has(unsigned int id) const;

    unsigned int load(const std::string& path);
    unsigned int get(unsigned int id);

private:
    std::unordered_map<unsigned int, unsigned int> textures; // OpenGL id
    unsigned int nextID = 0;

};