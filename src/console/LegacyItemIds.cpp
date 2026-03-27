#include "LegacyItemIds.h"

namespace LCEServer
{
    namespace
    {
        struct ItemSpec
        {
            int id = -1;
            int aux = 0;
        };

        std::string NormalizeItemToken(const std::string& token)
        {
            std::string out;
            out.reserve(token.size() + 8);

            std::string working = token;
            if (working.size() > 10 &&
                working.compare(0, 10, "minecraft:") == 0)
            {
                working = working.substr(10);
            }

            for (size_t i = 0; i < working.size(); ++i)
            {
                unsigned char c = static_cast<unsigned char>(working[i]);
                if (c == '-' || c == ' ')
                {
                    out.push_back('_');
                    continue;
                }

                if (c >= 'A' && c <= 'Z')
                {
                    if (!out.empty() && out.back() != '_')
                        out.push_back('_');
                    out.push_back(static_cast<char>(c - 'A' + 'a'));
                    continue;
                }

                out.push_back(static_cast<char>(std::tolower(c)));
            }

            return out;
        }

        const std::unordered_map<std::string, int>& LegacyItemIdMap()
        {
            static const std::unordered_map<std::string, int> kMap = []()
            {
                std::unordered_map<std::string, int> map;

                auto add = [&](const char* name, int id)
                {
                    map.emplace(NormalizeItemToken(name), id);
                };

                add("apple", 260);
                add("apple_gold", 322);
                add("arrow", 262);
                add("bed", 355);
                add("beef_cooked", 364);
                add("beef_raw", 363);
                add("blazePowder", 377);
                add("blazeRod", 369);
                add("boat", 333);
                add("bone", 352);
                add("book", 340);
                add("boots_chain", 305);
                add("boots_diamond", 313);
                add("boots_gold", 317);
                add("boots_iron", 309);
                add("boots_leather", 301);
                add("bow", 261);
                add("bowl", 281);
                add("bread", 297);
                add("brewingStand", 379);
                add("brick", 336);
                add("bucket_empty", 325);
                add("bucket_lava", 327);
                add("bucket_milk", 335);
                add("bucket_water", 326);
                add("cake", 354);
                add("carrotGolden", 396);
                add("carrotOnAStick", 398);
                add("carrots", 391);
                add("cauldron", 380);
                add("chestplate_chain", 303);
                add("chestplate_diamond", 311);
                add("chestplate_gold", 315);
                add("chestplate_iron", 307);
                add("chestplate_leather", 299);
                add("chicken_cooked", 366);
                add("chicken_raw", 365);
                add("clay", 337);
                add("clock", 347);
                add("coal", 263);
                add("comparator", 404);
                add("compass", 345);
                add("cookie", 357);
                add("diamond", 264);
                add("door_iron", 330);
                add("door_wood", 324);
                add("dye_powder", 351);
                add("egg", 344);
                add("emerald", 388);
                add("emptyMap", 395);
                add("enchantedBook", 403);
                add("enderPearl", 368);
                add("expBottle", 384);
                add("eyeOfEnder", 381);
                add("feather", 288);
                add("fermentedSpiderEye", 376);
                add("fireball", 385);
                add("fireworks", 401);
                add("fireworksCharge", 402);
                add("fish_cooked", 350);
                add("fish_raw", 349);
                add("fishingRod", 346);
                add("flint", 318);
                add("flintAndSteel", 259);
                add("flowerPot", 390);
                add("ghastTear", 370);
                add("glassBottle", 374);
                add("goldIngot", 266);
                add("goldNugget", 371);
                add("gunpowder", 289);
                add("hatchet_diamond", 279);
                add("hatchet_gold", 286);
                add("hatchet_iron", 258);
                add("hatchet_stone", 275);
                add("hatchet_wood", 271);
                add("helmet_chain", 302);
                add("helmet_diamond", 310);
                add("helmet_gold", 314);
                add("helmet_iron", 306);
                add("helmet_leather", 298);
                add("hoe_diamond", 293);
                add("hoe_gold", 294);
                add("hoe_iron", 292);
                add("hoe_stone", 291);
                add("hoe_wood", 290);
                add("horseArmorDiamond", 419);
                add("horseArmorGold", 418);
                add("horseArmorMetal", 417);
                add("ironIngot", 265);
                add("itemFrame", 389);
                add("lead", 420);
                add("leather", 334);
                add("leggings_chain", 304);
                add("leggings_diamond", 312);
                add("leggings_gold", 316);
                add("leggings_iron", 308);
                add("leggings_leather", 300);
                add("magmaCream", 378);
                add("map", 358);
                add("melon", 360);
                add("minecart", 328);
                add("minecart_chest", 342);
                add("minecart_furnace", 343);
                add("minecart_hopper", 408);
                add("minecart_tnt", 407);
                add("mushroomStew", 282);
                add("nameTag", 421);
                add("netherQuartz", 406);
                add("netherStar", 399);
                add("netherbrick", 405);
                add("netherwart_seeds", 372);
                add("painting", 321);
                add("paper", 339);
                add("pickAxe_diamond", 278);
                add("pickAxe_gold", 285);
                add("pickAxe_iron", 257);
                add("pickAxe_stone", 274);
                add("pickAxe_wood", 270);
                add("porkChop_cooked", 320);
                add("porkChop_raw", 319);
                add("potato", 392);
                add("potatoBaked", 393);
                add("potatoPoisonous", 394);
                add("potion", 373);
                add("pumpkinPie", 400);
                add("record_01", 2256);
                add("record_02", 2257);
                add("record_03", 2258);
                add("record_04", 2259);
                add("record_05", 2260);
                add("record_06", 2261);
                add("record_07", 2262);
                add("record_08", 2267);
                add("record_09", 2263);
                add("record_10", 2264);
                add("record_11", 2265);
                add("record_12", 2266);
                add("redStone", 331);
                add("reeds", 338);
                add("repeater", 356);
                add("rotten_flesh", 367);
                add("saddle", 329);
                add("seeds_melon", 362);
                add("seeds_pumpkin", 361);
                add("seeds_wheat", 295);
                add("shears", 359);
                add("shovel_diamond", 277);
                add("shovel_gold", 284);
                add("shovel_iron", 256);
                add("shovel_stone", 273);
                add("shovel_wood", 269);
                add("sign", 323);
                add("skull", 397);
                add("slimeBall", 341);
                add("snowBall", 332);
                add("spawnEgg", 383);
                add("speckledMelon", 382);
                add("spiderEye", 375);
                add("stick", 280);
                add("string", 287);
                add("sugar", 353);
                add("sword_diamond", 276);
                add("sword_gold", 283);
                add("sword_iron", 267);
                add("sword_stone", 272);
                add("sword_wood", 268);
                add("wheat", 296);
                add("writingBook", 130);
                add("writtenBook", 131);
                add("yellowDust", 348);

                add("diamond_chestplate", 311);
                add("diamond_helmet", 310);
                add("diamond_leggings", 312);
                add("diamond_boots", 313);
                add("iron_chestplate", 307);
                add("iron_helmet", 306);
                add("iron_leggings", 308);
                add("iron_boots", 309);
                add("gold_chestplate", 315);
                add("golden_chestplate", 315);
                add("gold_helmet", 314);
                add("golden_helmet", 314);
                add("gold_leggings", 316);
                add("golden_leggings", 316);
                add("gold_boots", 317);
                add("golden_boots", 317);
                add("chainmail_chestplate", 303);
                add("chainmail_helmet", 302);
                add("chainmail_leggings", 304);
                add("chainmail_boots", 305);
                add("leather_chestplate", 299);
                add("leather_helmet", 298);
                add("leather_leggings", 300);
                add("leather_boots", 301);
                add("iron_sword", 267);
                add("wooden_sword", 268);
                add("stone_sword", 272);
                add("diamond_sword", 276);
                add("gold_sword", 283);
                add("golden_sword", 283);
                add("iron_shovel", 256);
                add("wooden_shovel", 269);
                add("stone_shovel", 273);
                add("diamond_shovel", 277);
                add("gold_shovel", 284);
                add("golden_shovel", 284);
                add("iron_pickaxe", 257);
                add("wooden_pickaxe", 270);
                add("stone_pickaxe", 274);
                add("diamond_pickaxe", 278);
                add("gold_pickaxe", 285);
                add("golden_pickaxe", 285);
                add("iron_axe", 258);
                add("wooden_axe", 271);
                add("stone_axe", 275);
                add("diamond_axe", 279);
                add("gold_axe", 286);
                add("golden_axe", 286);
                add("iron_hoe", 292);
                add("wooden_hoe", 290);
                add("stone_hoe", 291);
                add("diamond_hoe", 293);
                add("gold_hoe", 294);
                add("golden_hoe", 294);
                add("golden_apple", 322);
                add("golden_carrot", 396);
                add("carrot_on_a_stick", 398);
                add("ender_pearl", 368);
                add("blaze_powder", 377);
                add("blaze_rod", 369);
                add("spider_eye", 375);
                add("fermented_spider_eye", 376);
                add("ghast_tear", 370);
                add("magma_cream", 378);
                add("glass_bottle", 374);
                add("gold_ingot", 266);
                add("gold_nugget", 371);
                add("iron_ingot", 265);
                add("item_frame", 389);
                add("name_tag", 421);
                add("nether_star", 399);
                add("nether_quartz", 406);
                add("nether_brick", 405);
                add("raw_beef", 363);
                add("cooked_beef", 364);
                add("raw_chicken", 365);
                add("cooked_chicken", 366);
                add("raw_porkchop", 319);
                add("cooked_porkchop", 320);
                add("raw_fish", 349);
                add("cooked_fish", 350);
                add("baked_potato", 393);
                add("poisonous_potato", 394);
                add("experience_bottle", 384);
                add("glowstone_dust", 348);
                add("redstone", 331);
                add("empty_map", 395);
                add("enchanted_book", 403);
                add("fire_charge", 385);
                add("firework_rocket", 401);
                add("firework_star", 402);
                add("horse_armor_iron", 417);
                add("iron_horse_armor", 417);
                add("horse_armor_gold", 418);
                add("golden_horse_armor", 418);
                add("horse_armor_diamond", 419);
                add("nether_wart", 372);
                add("water_bucket", 326);
                add("lava_bucket", 327);
                add("milk_bucket", 335);
                add("minecart_with_chest", 342);
                add("chest_minecart", 342);
                add("minecart_with_furnace", 343);
                add("furnace_minecart", 343);
                add("tnt_minecart", 407);
                add("hopper_minecart", 408);
                add("oak_sign", 323);
                add("oak_door", 324);

                return map;
            }();

            return kMap;
        }

        const std::unordered_map<std::string, ItemSpec>& LegacyBlockItemMap()
        {
            static const std::unordered_map<std::string, ItemSpec> kMap = []()
            {
                std::unordered_map<std::string, ItemSpec> map;
                auto add = [&](const char* name, int id, int aux = 0)
                {
                    map.emplace(NormalizeItemToken(name), ItemSpec{ id, aux });
                };

                add("stone", 1);
                add("grass", 2);
                add("dirt", 3);
                add("cobblestone", 4);
                add("planks", 5, 0);
                add("oak_planks", 5, 0);
                add("spruce_planks", 5, 1);
                add("birch_planks", 5, 2);
                add("jungle_planks", 5, 3);
                add("acacia_planks", 5, 4);
                add("dark_oak_planks", 5, 5);
                add("sapling", 6, 0);
                add("oak_sapling", 6, 0);
                add("spruce_sapling", 6, 1);
                add("birch_sapling", 6, 2);
                add("jungle_sapling", 6, 3);
                add("acacia_sapling", 6, 4);
                add("dark_oak_sapling", 6, 5);
                add("bedrock", 7);
                add("sand", 12, 0);
                add("red_sand", 12, 1);
                add("gravel", 13);
                add("gold_ore", 14);
                add("iron_ore", 15);
                add("coal_ore", 16);
                add("log", 17, 0);
                add("wooden_log", 17, 0);
                add("oak_log", 17, 0);
                add("spruce_log", 17, 1);
                add("birch_log", 17, 2);
                add("jungle_log", 17, 3);
                add("oak_wood", 17, 12);
                add("spruce_wood", 17, 13);
                add("birch_wood", 17, 14);
                add("jungle_wood", 17, 15);
                add("leaves", 18, 0);
                add("oak_leaves", 18, 0);
                add("spruce_leaves", 18, 1);
                add("birch_leaves", 18, 2);
                add("jungle_leaves", 18, 3);
                add("glass", 20);
                add("lapis_ore", 21);
                add("lapis_block", 22);
                add("dispenser", 23);
                add("sandstone", 24, 0);
                add("chiseled_sandstone", 24, 1);
                add("cut_sandstone", 24, 2);
                add("note_block", 25);
                add("powered_rail", 27);
                add("detector_rail", 28);
                add("sticky_piston", 29);
                add("cobweb", 30);
                add("dead_bush", 32);
                add("piston", 33);
                add("wool", 35, 0);
                add("white_wool", 35, 0);
                add("orange_wool", 35, 1);
                add("magenta_wool", 35, 2);
                add("light_blue_wool", 35, 3);
                add("yellow_wool", 35, 4);
                add("lime_wool", 35, 5);
                add("pink_wool", 35, 6);
                add("gray_wool", 35, 7);
                add("light_gray_wool", 35, 8);
                add("cyan_wool", 35, 9);
                add("purple_wool", 35, 10);
                add("blue_wool", 35, 11);
                add("brown_wool", 35, 12);
                add("green_wool", 35, 13);
                add("red_wool", 35, 14);
                add("black_wool", 35, 15);
                add("Playerdelion", 37);
                add("poppy", 38);
                add("brown_mushroom", 39);
                add("red_mushroom", 40);
                add("gold_block", 41);
                add("iron_block", 42);
                add("stone_slab", 44, 0);
                add("sandstone_slab", 44, 1);
                add("cobblestone_slab", 44, 3);
                add("brick_slab", 44, 4);
                add("stone_brick_slab", 44, 5);
                add("nether_brick_slab", 44, 6);
                add("quartz_slab", 44, 7);
                add("bricks", 45);
                add("tnt", 46);
                add("bookshelf", 47);
                add("mossy_cobblestone", 48);
                add("obsidian", 49);
                add("torch", 50);
                add("mob_spawner", 52);
                add("oak_stairs", 53);
                add("chest", 54);
                add("diamond_ore", 56);
                add("diamond_block", 57);
                add("crafting_table", 58);
                add("farmland", 60);
                add("furnace", 61);
                add("ladder", 65);
                add("rail", 66);
                add("cobblestone_stairs", 67);
                add("lever", 69);
                add("stone_pressure_plate", 70);
                add("wooden_pressure_plate", 72);
                add("oak_pressure_plate", 72);
                add("redstone_ore", 73);
                add("redstone_torch", 76);
                add("stone_button", 77);
                add("snow", 78);
                add("ice", 79);
                add("snow_block", 80);
                add("cactus", 81);
                add("clay_block", 82);
                add("sugar_cane", 83);
                add("jukebox", 84);
                add("oak_fence", 85);
                add("netherrack", 87);
                add("soul_sand", 88);
                add("glowstone", 89);
                add("jack_o_lantern", 91);
                add("stone_bricks", 98, 0);
                add("mossy_stone_bricks", 98, 1);
                add("cracked_stone_bricks", 98, 2);
                add("chiseled_stone_bricks", 98, 3);
                add("brown_mushroom_block", 99);
                add("red_mushroom_block", 100);
                add("iron_bars", 101);
                add("glass_pane", 102);
                add("melon_block", 103);
                add("vine", 106);
                add("oak_fence_gate", 107);
                add("brick_stairs", 108);
                add("stone_brick_stairs", 109);
                add("mycelium", 110);
                add("lily_pad", 111);
                add("nether_bricks", 112);
                add("nether_brick_fence", 113);
                add("nether_brick_stairs", 114);
                add("enchanting_table", 116);
                add("end_stone", 121);
                add("sandstone_stairs", 128);
                add("emerald_ore", 129);
                add("ender_chest", 130);
                add("tripwire_hook", 131);
                add("emerald_block", 133);
                add("spruce_stairs", 134);
                add("birch_stairs", 135);
                add("jungle_stairs", 136);
                add("beacon", 138);
                add("cobblestone_wall", 139, 0);
                add("mossy_cobblestone_wall", 139, 1);
                add("flower_pot", 140);
                add("anvil", 145);
                add("trapped_chest", 146);
                add("light_weighted_pressure_plate", 147);
                add("heavy_weighted_pressure_plate", 148);
                add("daylight_detector", 151);
                add("redstone_block", 152);
                add("quartz_ore", 153);
                add("hopper", 154);
                add("quartz_block", 155, 0);
                add("chiseled_quartz_block", 155, 1);
                add("quartz_pillar", 155, 2);
                add("quartz_stairs", 156);
                add("activator_rail", 157);
                add("dropper", 158);
                add("white_stained_hardened_clay", 159, 0);
                add("orange_stained_hardened_clay", 159, 1);
                add("magenta_stained_hardened_clay", 159, 2);
                add("light_blue_stained_hardened_clay", 159, 3);
                add("yellow_stained_hardened_clay", 159, 4);
                add("lime_stained_hardened_clay", 159, 5);
                add("pink_stained_hardened_clay", 159, 6);
                add("gray_stained_hardened_clay", 159, 7);
                add("light_gray_stained_hardened_clay", 159, 8);
                add("cyan_stained_hardened_clay", 159, 9);
                add("purple_stained_hardened_clay", 159, 10);
                add("blue_stained_hardened_clay", 159, 11);
                add("brown_stained_hardened_clay", 159, 12);
                add("green_stained_hardened_clay", 159, 13);
                add("red_stained_hardened_clay", 159, 14);
                add("black_stained_hardened_clay", 159, 15);
                add("white_stained_glass_pane", 160, 0);
                add("orange_stained_glass_pane", 160, 1);
                add("magenta_stained_glass_pane", 160, 2);
                add("light_blue_stained_glass_pane", 160, 3);
                add("yellow_stained_glass_pane", 160, 4);
                add("lime_stained_glass_pane", 160, 5);
                add("pink_stained_glass_pane", 160, 6);
                add("gray_stained_glass_pane", 160, 7);
                add("light_gray_stained_glass_pane", 160, 8);
                add("cyan_stained_glass_pane", 160, 9);
                add("purple_stained_glass_pane", 160, 10);
                add("blue_stained_glass_pane", 160, 11);
                add("brown_stained_glass_pane", 160, 12);
                add("green_stained_glass_pane", 160, 13);
                add("red_stained_glass_pane", 160, 14);
                add("black_stained_glass_pane", 160, 15);
                add("acacia_leaves", 161, 0);
                add("dark_oak_leaves", 161, 1);
                add("log2", 162, 0);
                add("acacia_log", 162, 0);
                add("dark_oak_log", 162, 1);
                add("acacia_wood", 162, 12);
                add("dark_oak_wood", 162, 13);
                add("acacia_stairs", 163);
                add("dark_oak_stairs", 164);
                add("hay_block", 170);
                add("carpet", 171, 0);
                add("white_carpet", 171, 0);
                add("orange_carpet", 171, 1);
                add("magenta_carpet", 171, 2);
                add("light_blue_carpet", 171, 3);
                add("yellow_carpet", 171, 4);
                add("lime_carpet", 171, 5);
                add("pink_carpet", 171, 6);
                add("gray_carpet", 171, 7);
                add("light_gray_carpet", 171, 8);
                add("cyan_carpet", 171, 9);
                add("purple_carpet", 171, 10);
                add("blue_carpet", 171, 11);
                add("brown_carpet", 171, 12);
                add("green_carpet", 171, 13);
                add("red_carpet", 171, 14);
                add("black_carpet", 171, 15);

                return map;
            }();

            return kMap;
        }
    }

    bool TryResolveLegacyItemSpec(const std::string& token, int& outItemId, int& outAux)
    {
        const std::string key = NormalizeItemToken(token);

        int itemId = -1;
        if (const auto& itemMap = LegacyItemIdMap(); itemMap.find(key) != itemMap.end())
        {
            itemId = itemMap.at(key);
            outItemId = itemId;
            outAux = 0;
            return true;
        }

        const auto& blockMap = LegacyBlockItemMap();
        auto blockIt = blockMap.find(key);
        if (blockIt == blockMap.end())
            return false;

        outItemId = blockIt->second.id;
        outAux = blockIt->second.aux;
        return true;
    }

    bool TryResolveLegacyItemId(const std::string& token, int& outItemId)
    {
        int aux = 0;
        if (!TryResolveLegacyItemSpec(token, outItemId, aux))
            return false;
        return true;
    }
}
