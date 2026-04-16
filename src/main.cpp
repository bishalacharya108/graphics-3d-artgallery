#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../include/json.hpp"

using json = nlohmann::json;

struct Artwork {
    int id;
    std::string title;
    std::string category;
    std::string description;
    std::string image;
    int width;
    int height;
    std::string source;
    std::string license;
};

std::vector<Artwork> loadGallery(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + path);
    }

    json j;
    file >> j;

    std::vector<Artwork> artworks;

    for (const auto& item : j) {
        Artwork art{};
        art.id = item.value("id", 0);
        art.title = item.value("title", "Untitled");
        art.category = item.value("category", "General");
        art.description = item.value("description", "");
        art.image = item.value("image", "");
        art.width = item.value("width", 0);
        art.height = item.value("height", 0);
        art.source = item.value("source", "Unknown");
        art.license = item.value("license", "public_domain");

        artworks.push_back(art);
    }

    return artworks;
}

int main() {
    try {
        std::string path = "data/gallery.json";

        auto artworks = loadGallery(path);

        std::cout << "Loaded " << artworks.size() << " artworks\n\n";

        for (const auto& art : artworks) {
            std::cout << "ID: " << art.id << "\n";
            std::cout << "Title: " << art.title << "\n";
            std::cout << "Category: " << art.category << "\n";
            std::cout << "Image: " << art.image << "\n";
            std::cout << "Size: " << art.width << " x " << art.height << "\n";
            std::cout << "Source: " << art.source << "\n";
            std::cout << "License: " << art.license << "\n";
            std::cout << "--------------------------\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
