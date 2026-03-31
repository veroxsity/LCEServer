// LCEServer — server.properties loader/saver
#include "ServerConfig.h"
#include <charconv>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

namespace LCEServer
{
    // Trim whitespace
    static std::string Trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static std::string ToLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

    static bool TryParseBool(const std::string& v, bool& outValue)
    {
        std::string l = ToLower(Trim(v));
        if (l == "true" || l == "1" || l == "yes")
        {
            outValue = true;
            return true;
        }
        if (l == "false" || l == "0" || l == "no")
        {
            outValue = false;
            return true;
        }
        return false;
    }

    static bool TryParseInt(const std::string& v, int& outValue)
    {
        const std::string trimmed = Trim(v);
        if (trimmed.empty())
            return false;

        const char* begin = trimmed.data();
        const char* end = begin + trimmed.size();
        auto result = std::from_chars(begin, end, outValue);
        return result.ec == std::errc() && result.ptr == end;
    }

    static std::wstring Utf8ToWide(const std::string& s)
    {
        if (s.empty()) return L"";
        int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        if (needed <= 0)
            return L"";
        std::wstring out(needed, 0);
        if (MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], needed) <= 0)
            return L"";
        return out;
    }

    static std::string WideToUtf8(const std::wstring& s)
    {
        if (s.empty()) return "";
        int needed = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
            return "";
        std::string out(needed, 0);
        if (WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], needed, nullptr, nullptr) <= 0)
            return "";
        return out;
    }

    template <typename T>
    static T ClampValue(T value, T minValue, T maxValue)
    {
        if (value < minValue)
            return minValue;
        if (value > maxValue)
            return maxValue;
        return value;
    }

    uint32_t ServerConfig::BuildServerSettings() const
    {
        // This matches the bitfield used in the source's eGameHostOption_All / MAKE_SERVER_SETTINGS
        // For now return 0 — full bitfield construction will be implemented once we have
        // the exact bit positions from the source's App_enums.h
        return 0;
    }

    ServerConfig ServerConfig::Load(const std::string& path)
    {
        ServerConfig cfg;

        std::ifstream file(path);
        if (!file.is_open())
        {
            Logger::Info("Config", "server.properties not found, creating with defaults");
            cfg.Save(path);
            return cfg;
        }

        // Read all key=value pairs, preserving unknown keys
        std::map<std::string, std::string> props;
        std::string line;
        while (std::getline(file, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == '#') continue;
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = Trim(line.substr(0, eq));
            std::string val = Trim(line.substr(eq + 1));
            props[key] = val;
        }
        file.close();

        auto get = [&](const char* k, const std::string& def) -> std::string {
            auto it = props.find(k);
            return (it != props.end()) ? it->second : def;
        };

        auto parseIntProperty = [&](const char* key, int defaultValue) -> int
        {
            auto it = props.find(key);
            if (it == props.end())
                return defaultValue;

            int parsedValue = defaultValue;
            if (!TryParseInt(it->second, parsedValue))
            {
                Logger::Warn("Config",
                    "Invalid integer for %s='%s'; using %d",
                    key, it->second.c_str(), defaultValue);
                return defaultValue;
            }
            return parsedValue;
        };

        auto parseBoolProperty = [&](const char* key, bool defaultValue) -> bool
        {
            auto it = props.find(key);
            if (it == props.end())
                return defaultValue;

            bool parsedValue = defaultValue;
            if (!TryParseBool(it->second, parsedValue))
            {
                Logger::Warn("Config",
                    "Invalid boolean for %s='%s'; using %s",
                    key, it->second.c_str(), defaultValue ? "true" : "false");
                return defaultValue;
            }
            return parsedValue;
        };

        // Network
        cfg.serverPort     = ClampValue(parseIntProperty("server-port", DEFAULT_PORT), 1, 65535);
        cfg.serverIp       = get("server-ip", "0.0.0.0");
        cfg.serverName     = get("server-name", "LCEServer");
        cfg.lanAdvertise   = parseBoolProperty("lan-advertise", true);

        // World
        cfg.levelName      = Utf8ToWide(get("level-name", "world"));
        cfg.levelId        = get("level-id", "");
        cfg.levelSeed      = get("level-seed", "");
        cfg.levelType      = get("level-type", "flat");
        cfg.maxBuildHeight = parseIntProperty(
            "max-build-height",
            LEGACY_WORLD_HEIGHT);
        if (cfg.maxBuildHeight <= 0 || cfg.maxBuildHeight > LEGACY_WORLD_HEIGHT)
            cfg.maxBuildHeight = LEGACY_WORLD_HEIGHT;

        // Game settings
        cfg.gamemode            = ClampValue(parseIntProperty("gamemode", 0), 0, 2);
        cfg.difficulty          = ClampValue(parseIntProperty("difficulty", 1), 0, 3);
        cfg.pvp                 = parseBoolProperty("pvp", true);
        cfg.trustPlayers        = parseBoolProperty("trust-players", false);
        cfg.fireSpreads         = parseBoolProperty("fire-spreads", true);
        cfg.tnt                 = parseBoolProperty("tnt", true);
        cfg.spawnAnimals        = parseBoolProperty("spawn-animals", true);
        cfg.spawnNpcs           = parseBoolProperty("spawn-npcs", true);
        cfg.spawnMonsters       = parseBoolProperty("spawn-monsters", true);
        cfg.allowFlight         = parseBoolProperty("allow-flight", true);
        cfg.allowNether         = parseBoolProperty("allow-nether", true);
        cfg.generateStructures  = parseBoolProperty("generate-structures", true);
        cfg.bonusChest          = parseBoolProperty("bonus-chest", false);
        cfg.friendsOfFriends    = parseBoolProperty("friends-of-friends", true);
        cfg.gamertags           = parseBoolProperty("gamertags", true);
        cfg.bedrockFog          = parseBoolProperty("bedrock-fog", false);
        cfg.mobGriefing         = parseBoolProperty("mob-griefing", true);
        cfg.keepInventory       = parseBoolProperty("keep-inventory", false);
        cfg.naturalRegeneration = parseBoolProperty("natural-regeneration", true);
        cfg.doDaylightCycle     = parseBoolProperty("do-daylight-cycle", true);

        // Server
        cfg.maxPlayers       = ClampValue(parseIntProperty("max-players", 8), 1, MAX_PLAYERS_HARD);
        int vd = parseIntProperty("view-distance", 4);
        cfg.viewDistance     = ClampValue(vd, 2, 16);
        int cpt = parseIntProperty("chunks-per-tick", 10);
        cfg.chunksPerTick    = ClampValue(cpt, 1, 64);
        cfg.motd             = get("motd", "A Minecraft LCE Server");
        cfg.autosaveInterval = ClampValue(parseIntProperty("autosave-interval", 300), 0, 86400);
        cfg.disableSaving    = parseBoolProperty("disable-saving", false);
        cfg.whiteList        = parseBoolProperty("white-list", false);

        // Log level
        std::string ll = ToLower(get("log-level", "info"));
        if      (ll == "debug") cfg.logLevel = LogLevel::Debug;
        else if (ll == "warn")  cfg.logLevel = LogLevel::Warn;
        else if (ll == "error") cfg.logLevel = LogLevel::Error;
        else                    cfg.logLevel = LogLevel::Info;

        // RCON
        cfg.rconPort     = ClampValue(parseIntProperty("rcon-port", 25575), 1, 65535);
        cfg.rconPassword = get("rcon-password", "");

        return cfg;
    }

    bool ServerConfig::Save(const std::string& path) const
    {
        std::ofstream f(path);
        if (!f.is_open()) return false;

        auto b = [](bool v) -> const char* { return v ? "true" : "false"; };
        auto ll = [](LogLevel l) -> const char* {
            switch(l) {
                case LogLevel::Debug: return "debug";
                case LogLevel::Warn:  return "warn";
                case LogLevel::Error: return "error";
                default: return "info";
            }
        };

        f << "# LCEServer Configuration\n\n";
        f << "# Network\n";
        f << "server-port=" << serverPort << "\n";
        f << "server-ip=" << serverIp << "\n";
        f << "server-name=" << serverName << "\n";
        f << "lan-advertise=" << b(lanAdvertise) << "\n\n";

        f << "# World\n";
        f << "level-name=" << WideToUtf8(levelName) << "\n";
        f << "level-id=" << levelId << "\n";
        f << "level-seed=" << levelSeed << "\n";
        f << "level-type=" << levelType << "\n";
        f << "max-build-height=" << maxBuildHeight << "\n\n";
        f << "# Game settings\n";
        f << "gamemode=" << gamemode << "\n";
        f << "difficulty=" << difficulty << "\n";
        f << "pvp=" << b(pvp) << "\n";
        f << "trust-players=" << b(trustPlayers) << "\n";
        f << "fire-spreads=" << b(fireSpreads) << "\n";
        f << "tnt=" << b(tnt) << "\n";
        f << "spawn-animals=" << b(spawnAnimals) << "\n";
        f << "spawn-npcs=" << b(spawnNpcs) << "\n";
        f << "spawn-monsters=" << b(spawnMonsters) << "\n";
        f << "allow-flight=" << b(allowFlight) << "\n";
        f << "allow-nether=" << b(allowNether) << "\n";
        f << "generate-structures=" << b(generateStructures) << "\n";
        f << "bonus-chest=" << b(bonusChest) << "\n";
        f << "friends-of-friends=" << b(friendsOfFriends) << "\n";
        f << "gamertags=" << b(gamertags) << "\n";
        f << "bedrock-fog=" << b(bedrockFog) << "\n";
        f << "mob-griefing=" << b(mobGriefing) << "\n";
        f << "keep-inventory=" << b(keepInventory) << "\n";
        f << "natural-regeneration=" << b(naturalRegeneration) << "\n";
        f << "do-daylight-cycle=" << b(doDaylightCycle) << "\n\n";

        f << "# Server\n";
        f << "max-players=" << maxPlayers << "\n";
        f << "view-distance=" << viewDistance << "\n";
        f << "chunks-per-tick=" << chunksPerTick << "\n";
        f << "motd=" << motd << "\n";
        f << "autosave-interval=" << autosaveInterval << "\n";
        f << "log-level=" << ll(logLevel) << "\n";
        f << "disable-saving=" << b(disableSaving) << "\n";
        f << "white-list=" << b(whiteList) << "\n";

        f << "\n# RCON\n";
        f << "rcon-port=" << rconPort << "\n";
        f << "rcon-password=" << rconPassword << "\n";

        f.close();
        return true;
    }
}
