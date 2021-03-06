#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#include "Exception.hpp"
#include "Subsystem.hpp"
#include "ZipReader.hpp"
#include "Directory.hpp"
#include "Tileset.hpp"
#include "Object.hpp"
#include "Characterset.hpp"
#include "NPC.hpp"
#include "Animation.hpp"
#include "Map.hpp"
#include "Background.hpp"
#include "Font.hpp"
#include "Icon.hpp"
#include "Sound.hpp"
#include "Music.hpp"

#include <string>
#include <map>
#include <vector>

class ResourcesException : public Exception {
public:
    ResourcesException(const char *msg) : Exception(msg) { }
    ResourcesException(const std::string& msg) : Exception(msg) { }
};

class Tournament;

class Resources {
private:
    Resources(const Resources&);
    Resources& operator=(const Resources&);

public:
    struct LoadedPak {
        LoadedPak(const std::string& pak_name, const std::string& pak_short_name,
            const std::string& pak_hash, bool from_home_dir)
            : pak_name(pak_name), pak_short_name(pak_short_name), pak_hash(pak_hash),
              from_home_dir(from_home_dir) { }

        std::string pak_name;
        std::string pak_short_name;
        std::string pak_hash;
        bool from_home_dir;
    };

    typedef std::vector<LoadedPak> LoadedPaks;

    struct ResourceObject {
        ResourceObject(void *object, bool base_resource) : object(object), base_resource(base_resource) { }
        void *object;
        bool base_resource;
    };

    typedef std::vector<ResourceObject> ResourceObjects;

    Resources(Subsystem& subystem, const std::string& resource_directory) throw (ResourcesException);
    virtual ~Resources();

    void reload_resources() throw (ResourcesException);
    void update_tile_index(double diff, Tileset *tileset);
    const std::string& get_resource_directory() const;

    Tileset *get_tileset(const std::string& name) throw (ResourcesException);
    Object *get_object(const std::string& name) throw (ResourcesException);
    Characterset *get_characterset(const std::string& name) throw (ResourcesException);
    NPC *get_npc(const std::string& name) throw (ResourcesException);
    Animation *get_animation(const std::string& name) throw (ResourcesException);
    Map *get_map(const std::string& name) throw (ResourcesException);
    Background *get_background(const std::string& name) throw (ResourcesException);
    Font *get_font(const std::string& name) throw (ResourcesException);
    Icon *get_icon(const std::string& name) throw (ResourcesException);
    Sound *get_sound(const std::string& name) throw (ResourcesException);
    Music *get_music(const std::string& name) throw (ResourcesException);
    Properties *get_game_settings(const std::string& name) throw (ResourcesException);

    ResourceObjects& get_tilesets();
    ResourceObjects& get_objects();
    ResourceObjects& get_charactersets();
    ResourceObjects& get_npcs();
    ResourceObjects& get_animations();
    ResourceObjects& get_maps();
    ResourceObjects& get_backgrounds();
    ResourceObjects& get_fonts();
    ResourceObjects& get_icons();
    ResourceObjects& get_sounds();
    ResourceObjects& get_musics();
    ResourceObjects& get_game_settings();

    const LoadedPaks& get_loaded_paks() const;

private:
    Subsystem& subsystem;
    std::string resource_directory;

    LoadedPaks loaded_paks;
    ResourceObjects tilesets;
    ResourceObjects objects;
    ResourceObjects charactersets;
    ResourceObjects npcs;
    ResourceObjects animations;
    ResourceObjects maps;
    ResourceObjects backgrounds;
    ResourceObjects fonts;
    ResourceObjects icons;
    ResourceObjects sounds;
    ResourceObjects musics;
    ResourceObjects game_settings;

    void read_tilesets(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_objects(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_charactersets(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_npcs(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_animations(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_maps(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_backgrounds(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_fonts(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_icons(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_sounds(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_musics(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);
    void read_game_settings(const std::string& directory, ZipReader *zip, bool base_resource) throw (Exception);

    void load_resources(bool home_paks_only) throw (ResourcesException);
    void read_all(const std::string& fdir, ZipReader *fzip, bool base_resource) throw (Exception);
    void destroy_resources(bool home_paks_only);
    void prepare_resources() throw (ResourcesException);
};

#endif
