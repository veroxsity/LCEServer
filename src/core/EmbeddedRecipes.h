#pragma once

#include <string>

namespace LCEServer
{
    namespace EmbeddedRecipes
    {
        inline constexpr const char* kBasics = R"JSON(
  { "recipe": "0", "result": "5:0:4", "ingredients": "17:0:1" },
  { "recipe": "1", "result": "5:2:4", "ingredients": "17:2:1" },
  { "recipe": "2", "result": "5:1:4", "ingredients": "17:1:1" },
  { "recipe": "3", "result": "5:3:4", "ingredients": "17:3:1" },
  { "recipe": "4", "result": "280:0:4", "ingredients": "5:*:2" },
  { "recipe": "25", "result": "359:0:1", "ingredients": "265:0:2" },
  { "recipe": "29", "result": "282:0:1", "ingredients": "39:0:1|40:0:1|281:0:1" },
  { "recipe": "32", "result": "362:0:1", "ingredients": "360:0:1" },
  { "recipe": "33", "result": "361:0:4", "ingredients": "86:0:1" },
  { "recipe": "34", "result": "400:0:1", "ingredients": "86:0:1|353:0:1|344:0:1" },
  { "recipe": "36", "result": "376:0:1", "ingredients": "375:0:1|39:0:1|353:0:1" },
  { "recipe": "37", "result": "377:0:2", "ingredients": "369:0:1" },
  { "recipe": "38", "result": "378:0:1", "ingredients": "377:0:1|341:0:1" }
)JSON";

        inline constexpr const char* kStructures = R"JSON(
  { "recipe": "39", "result": "24:0:1", "ingredients": "12:0:4" },
  { "recipe": "40", "result": "24:2:4", "ingredients": "24:0:4" },
  { "recipe": "41", "result": "24:1:1", "ingredients": "44:1:2" },
  { "recipe": "42", "result": "155:1:1", "ingredients": "44:7:2" },
  { "recipe": "43", "result": "155:2:2", "ingredients": "155:0:2" },
  { "recipe": "44", "result": "58:0:1", "ingredients": "5:*:4" },
  { "recipe": "47", "result": "146:0:1", "ingredients": "54:0:1|131:0:1" },
  { "recipe": "49", "result": "98:0:4", "ingredients": "1:0:4" },
  { "recipe": "51", "result": "112:0:1", "ingredients": "405:0:4" },
  { "recipe": "156", "result": "80:0:1", "ingredients": "332:0:4" },
  { "recipe": "158", "result": "82:0:1", "ingredients": "337:0:4" },
  { "recipe": "159", "result": "45:0:1", "ingredients": "336:0:4" },
  { "recipe": "160", "result": "35:0:1", "ingredients": "287:0:4" }
)JSON";

        inline constexpr const char* kWoolRecolor = R"JSON(
  { "recipe": "93", "result": "35:0:1", "ingredients": "351:0:1|35:0:1" },
  { "recipe": "94", "result": "35:1:1", "ingredients": "351:1:1|35:0:1" },
  { "recipe": "95", "result": "35:2:1", "ingredients": "351:2:1|35:0:1" },
  { "recipe": "96", "result": "35:3:1", "ingredients": "351:3:1|35:0:1" },
  { "recipe": "97", "result": "35:4:1", "ingredients": "351:4:1|35:0:1" },
  { "recipe": "98", "result": "35:5:1", "ingredients": "351:5:1|35:0:1" },
  { "recipe": "99", "result": "35:6:1", "ingredients": "351:6:1|35:0:1" },
  { "recipe": "100", "result": "35:7:1", "ingredients": "351:7:1|35:0:1" },
  { "recipe": "101", "result": "35:8:1", "ingredients": "351:8:1|35:0:1" },
  { "recipe": "102", "result": "35:9:1", "ingredients": "351:9:1|35:0:1" },
  { "recipe": "103", "result": "35:10:1", "ingredients": "351:10:1|35:0:1" },
  { "recipe": "104", "result": "35:11:1", "ingredients": "351:11:1|35:0:1" },
  { "recipe": "105", "result": "35:12:1", "ingredients": "351:12:1|35:0:1" },
  { "recipe": "106", "result": "35:13:1", "ingredients": "351:13:1|35:0:1" },
  { "recipe": "107", "result": "35:14:1", "ingredients": "351:14:1|35:0:1" },
  { "recipe": "108", "result": "35:15:1", "ingredients": "351:15:1|35:0:1" }
)JSON";

        inline constexpr const char* kDyes = R"JSON(
  { "recipe": "125", "result": "351:11:2", "ingredients": "37:0:1" },
  { "recipe": "126", "result": "351:1:2", "ingredients": "38:0:1" },
  { "recipe": "127", "result": "351:15:3", "ingredients": "352:0:1" },
  { "recipe": "128", "result": "351:9:2", "ingredients": "351:1:1|351:15:1" },
  { "recipe": "129", "result": "351:14:2", "ingredients": "351:1:1|351:11:1" },
  { "recipe": "130", "result": "351:10:2", "ingredients": "351:2:1|351:15:1" },
  { "recipe": "131", "result": "351:8:2", "ingredients": "351:0:1|351:15:1" },
  { "recipe": "132", "result": "351:7:2", "ingredients": "351:8:1|351:15:1" },
  { "recipe": "133", "result": "351:7:3", "ingredients": "351:0:1|351:15:2" },
  { "recipe": "134", "result": "351:12:2", "ingredients": "351:4:1|351:15:1" },
  { "recipe": "135", "result": "351:6:2", "ingredients": "351:4:1|351:2:1" },
  { "recipe": "136", "result": "351:5:2", "ingredients": "351:4:1|351:1:1" },
  { "recipe": "137", "result": "351:13:2", "ingredients": "351:5:1|351:9:1" },
  { "recipe": "138", "result": "351:13:3", "ingredients": "351:4:1|351:1:1|351:9:1" },
  { "recipe": "139", "result": "351:13:4", "ingredients": "351:4:1|351:1:2|351:15:1" }
)JSON";

        inline constexpr const char* kUtilityAndTransport = R"JSON(
  { "recipe": "174", "result": "353:0:1", "ingredients": "338:0:1" },
  { "recipe": "180", "result": "342:0:1", "ingredients": "54:0:1|328:0:1" },
  { "recipe": "181", "result": "343:0:1", "ingredients": "61:0:1|328:0:1" },
  { "recipe": "182", "result": "407:0:1", "ingredients": "46:0:1|328:0:1" },
  { "recipe": "183", "result": "408:0:1", "ingredients": "154:0:1|328:0:1" },
  { "recipe": "186", "result": "398:0:1", "ingredients": "346:0:1|391:0:1" },
  { "recipe": "187", "result": "259:0:1", "ingredients": "265:0:1|318:0:1" }
)JSON";

        inline constexpr const char* kRedstoneAndMechanisms = R"JSON(
  { "recipe": "200", "result": "50:0:4", "ingredients": "263:1:1|280:0:1" },
  { "recipe": "201", "result": "50:0:4", "ingredients": "263:0:1|280:0:1" },
  { "recipe": "202", "result": "89:0:1", "ingredients": "348:0:4" },
  { "recipe": "203", "result": "155:0:1", "ingredients": "406:0:4" },
  { "recipe": "204", "result": "69:0:1", "ingredients": "4:0:1|280:0:1" },
  { "recipe": "206", "result": "76:0:1", "ingredients": "280:0:1|331:0:1" },
  { "recipe": "212", "result": "381:0:1", "ingredients": "368:0:1|377:0:1" },
  { "recipe": "213", "result": "385:0:3", "ingredients": "289:0:1|377:0:1|263:0:1" },
  { "recipe": "214", "result": "385:0:3", "ingredients": "289:0:1|377:0:1|263:1:1" },
  { "recipe": "218", "result": "77:0:1", "ingredients": "1:0:1" },
  { "recipe": "219", "result": "143:0:1", "ingredients": "5:*:1" },
  { "recipe": "220", "result": "72:0:1", "ingredients": "5:*:2" },
  { "recipe": "221", "result": "70:0:1", "ingredients": "1:0:2" },
  { "recipe": "222", "result": "148:0:1", "ingredients": "265:0:2" },
  { "recipe": "223", "result": "147:0:1", "ingredients": "266:0:2" },
  { "recipe": "228", "result": "91:0:1", "ingredients": "86:0:1|50:0:1" },
  { "recipe": "231", "result": "340:0:1", "ingredients": "339:0:3|334:0:1" },
  { "recipe": "256", "result": "29:0:1", "ingredients": "341:0:1|33:0:1" }
)JSON";

        inline constexpr const char* kDecompression = R"JSON(
  { "recipe": "237", "result": "266:0:9", "ingredients": "41:0:1" },
  { "recipe": "239", "result": "265:0:9", "ingredients": "42:0:1" },
  { "recipe": "241", "result": "264:0:9", "ingredients": "57:0:1" },
  { "recipe": "243", "result": "388:0:9", "ingredients": "133:0:1" },
  { "recipe": "245", "result": "351:4:9", "ingredients": "22:0:1" },
  { "recipe": "247", "result": "331:0:9", "ingredients": "152:0:1" },
  { "recipe": "249", "result": "263:0:9", "ingredients": "173:0:1" },
  { "recipe": "251", "result": "296:0:9", "ingredients": "170:0:1" },
  { "recipe": "253", "result": "371:0:9", "ingredients": "266:0:1" }
)JSON";

        inline std::string BuildInventoryRecipesJson()
        {
            const char* sections[] =
            {
                kBasics,
                kStructures,
                kWoolRecolor,
                kDyes,
                kUtilityAndTransport,
                kRedstoneAndMechanisms,
                kDecompression
            };

            std::string json = "[\n";
            for (size_t i = 0; i < (sizeof(sections) / sizeof(sections[0])); ++i)
            {
                if (i != 0)
                    json += ",\n";
                json += sections[i];
            }
            json += "\n]";
            return json;
        }
    }
}
