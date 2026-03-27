#pragma once

#include <string>

namespace LCEServer
{
    namespace EmbeddedRecipes
    {
        // Recipes 0-25 in original LCE registration order.
        inline constexpr const char* kCoreAndTools = R"JSON(
  { "recipe": "0", "result": "5:0:4", "ingredients": "17:0:1" },
  { "recipe": "1", "result": "5:2:4", "ingredients": "17:2:1" },
  { "recipe": "2", "result": "5:1:4", "ingredients": "17:1:1" },
  { "recipe": "3", "result": "5:3:4", "ingredients": "17:3:1" },
  { "recipe": "4", "result": "280:0:4", "ingredients": "5:*:1|5:*:1" },
  { "recipe": "5", "result": "270:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|280:*:1|280:*:1" },
  { "recipe": "6", "result": "269:0:1", "ingredients": "5:*:1|280:*:1|280:*:1" },
  { "recipe": "7", "result": "271:0:1", "ingredients": "5:*:1|5:*:1|280:*:1|280:*:1" },
  { "recipe": "8", "result": "290:0:1", "ingredients": "5:*:1|5:*:1|280:*:1|280:*:1" },
  { "recipe": "9", "result": "274:0:1", "ingredients": "4:*:1|4:*:1|4:*:1|280:*:1|280:*:1" },
  { "recipe": "10", "result": "273:0:1", "ingredients": "4:*:1|280:*:1|280:*:1" },
  { "recipe": "11", "result": "275:0:1", "ingredients": "4:*:1|4:*:1|280:*:1|280:*:1" },
  { "recipe": "12", "result": "291:0:1", "ingredients": "4:*:1|4:*:1|280:*:1|280:*:1" },
  { "recipe": "13", "result": "257:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|280:*:1|280:*:1" },
  { "recipe": "14", "result": "256:0:1", "ingredients": "265:*:1|280:*:1|280:*:1" },
  { "recipe": "15", "result": "258:0:1", "ingredients": "265:*:1|265:*:1|280:*:1|280:*:1" },
  { "recipe": "16", "result": "292:0:1", "ingredients": "265:*:1|265:*:1|280:*:1|280:*:1" },
  { "recipe": "17", "result": "278:0:1", "ingredients": "264:*:1|264:*:1|264:*:1|280:*:1|280:*:1" },
  { "recipe": "18", "result": "277:0:1", "ingredients": "264:*:1|280:*:1|280:*:1" },
  { "recipe": "19", "result": "279:0:1", "ingredients": "264:*:1|264:*:1|280:*:1|280:*:1" },
  { "recipe": "20", "result": "293:0:1", "ingredients": "264:*:1|264:*:1|280:*:1|280:*:1" },
  { "recipe": "21", "result": "285:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|280:*:1|280:*:1" },
  { "recipe": "22", "result": "284:0:1", "ingredients": "266:*:1|280:*:1|280:*:1" },
  { "recipe": "23", "result": "286:0:1", "ingredients": "266:*:1|266:*:1|280:*:1|280:*:1" },
  { "recipe": "24", "result": "294:0:1", "ingredients": "266:*:1|266:*:1|280:*:1|280:*:1" },
  { "recipe": "25", "result": "359:0:1", "ingredients": "265:*:1|265:*:1" }
)JSON";

        // Recipes 26-52 in original LCE registration order.
        inline constexpr const char* kFoodAndStructures = R"JSON(
  { "recipe": "26", "result": "322:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|266:*:1|260:*:1|266:*:1|266:*:1|266:*:1|266:*:1" },
  { "recipe": "27", "result": "322:1:1", "ingredients": "41:*:1|41:*:1|41:*:1|41:*:1|260:*:1|41:*:1|41:*:1|41:*:1|41:*:1" },
  { "recipe": "28", "result": "382:0:1", "ingredients": "371:*:1|371:*:1|371:*:1|371:*:1|360:*:1|371:*:1|371:*:1|371:*:1|371:*:1" },
  { "recipe": "29", "result": "282:0:1", "ingredients": "39:*:1|40:*:1|281:*:1" },
  { "recipe": "30", "result": "357:0:8", "ingredients": "296:*:1|351:3:1|296:*:1" },
  { "recipe": "31", "result": "103:0:1", "ingredients": "360:*:1|360:*:1|360:*:1|360:*:1|360:*:1|360:*:1|360:*:1|360:*:1|360:*:1" },
  { "recipe": "32", "result": "362:0:1", "ingredients": "360:*:1" },
  { "recipe": "33", "result": "361:0:4", "ingredients": "86:*:1" },
  { "recipe": "34", "result": "400:0:1", "ingredients": "86:*:1|353:*:1|344:*:1" },
  { "recipe": "35", "result": "396:0:1", "ingredients": "371:*:1|371:*:1|371:*:1|371:*:1|391:*:1|371:*:1|371:*:1|371:*:1|371:*:1" },
  { "recipe": "36", "result": "376:0:1", "ingredients": "375:*:1|39:*:1|353:*:1" },
  { "recipe": "37", "result": "377:0:2", "ingredients": "369:*:1" },
  { "recipe": "38", "result": "378:0:1", "ingredients": "377:*:1|341:*:1" },
  { "recipe": "39", "result": "24:0:1", "ingredients": "12:*:1|12:*:1|12:*:1|12:*:1" },
  { "recipe": "40", "result": "24:2:4", "ingredients": "24:0:1|24:0:1|24:0:1|24:0:1" },
  { "recipe": "41", "result": "24:1:1", "ingredients": "44:1:1|44:1:1" },
  { "recipe": "42", "result": "155:1:1", "ingredients": "44:7:1|44:7:1" },
  { "recipe": "43", "result": "155:2:2", "ingredients": "155:0:1|155:0:1" },
  { "recipe": "44", "result": "58:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "45", "result": "61:0:1", "ingredients": "4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1" },
  { "recipe": "46", "result": "54:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "47", "result": "146:0:1", "ingredients": "54:*:1|131:*:1" },
  { "recipe": "48", "result": "130:0:1", "ingredients": "49:*:1|49:*:1|49:*:1|49:*:1|381:*:1|49:*:1|49:*:1|49:*:1|49:*:1" },
  { "recipe": "49", "result": "98:0:4", "ingredients": "1:*:1|1:*:1|1:*:1|1:*:1" },
  { "recipe": "50", "result": "102:0:16", "ingredients": "20:*:1|20:*:1|20:*:1|20:*:1|20:*:1|20:*:1" },
  { "recipe": "51", "result": "112:0:1", "ingredients": "405:*:1|405:*:1|405:*:1|405:*:1" },
  { "recipe": "52", "result": "123:0:1", "ingredients": "331:*:1|331:*:1|331:*:1|331:*:1" }
)JSON";

        // Recipes 53-92 in original LCE registration order.
        inline constexpr const char* kWorkbenchBasics = R"JSON(
  { "recipe": "53", "result": "138:0:1", "ingredients": "20:*:1|20:*:1|20:*:1|20:*:1|399:*:1|20:*:1|49:*:1|49:*:1|49:*:1" },
  { "recipe": "54", "result": "355:0:1", "ingredients": "35:*:1|35:*:1|35:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "55", "result": "116:0:1", "ingredients": "340:*:1|264:*:1|49:*:1|264:*:1|49:*:1|49:*:1|49:*:1" },
  { "recipe": "56", "result": "145:0:1", "ingredients": "42:*:1|42:*:1|42:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "57", "result": "65:0:3", "ingredients": "280:*:1|280:*:1|280:*:1|280:*:1|280:*:1|280:*:1|280:*:1" },
  { "recipe": "58", "result": "107:0:1", "ingredients": "280:*:1|5:*:1|280:*:1|280:*:1|5:*:1|280:*:1" },
  { "recipe": "59", "result": "85:0:2", "ingredients": "280:*:1|280:*:1|280:*:1|280:*:1|280:*:1|280:*:1" },
  { "recipe": "60", "result": "113:0:6", "ingredients": "112:*:1|112:*:1|112:*:1|112:*:1|112:*:1|112:*:1" },
  { "recipe": "61", "result": "101:0:16", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "62", "result": "139:0:6", "ingredients": "4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1" },
  { "recipe": "63", "result": "139:1:6", "ingredients": "48:*:1|48:*:1|48:*:1|48:*:1|48:*:1|48:*:1" },
  { "recipe": "64", "result": "324:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "65", "result": "330:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "66", "result": "53:0:4", "ingredients": "5:0:1|5:0:1|5:0:1|5:0:1|5:0:1|5:0:1" },
  { "recipe": "67", "result": "96:0:2", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "68", "result": "67:0:4", "ingredients": "4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1" },
  { "recipe": "69", "result": "108:0:4", "ingredients": "45:*:1|45:*:1|45:*:1|45:*:1|45:*:1|45:*:1" },
  { "recipe": "70", "result": "109:0:4", "ingredients": "98:*:1|98:*:1|98:*:1|98:*:1|98:*:1|98:*:1" },
  { "recipe": "71", "result": "114:0:4", "ingredients": "112:*:1|112:*:1|112:*:1|112:*:1|112:*:1|112:*:1" },
  { "recipe": "72", "result": "128:0:4", "ingredients": "24:*:1|24:*:1|24:*:1|24:*:1|24:*:1|24:*:1" },
  { "recipe": "73", "result": "135:0:4", "ingredients": "5:2:1|5:2:1|5:2:1|5:2:1|5:2:1|5:2:1" },
  { "recipe": "74", "result": "134:0:4", "ingredients": "5:1:1|5:1:1|5:1:1|5:1:1|5:1:1|5:1:1" },
  { "recipe": "75", "result": "136:0:4", "ingredients": "5:3:1|5:3:1|5:3:1|5:3:1|5:3:1|5:3:1" },
  { "recipe": "76", "result": "156:0:4", "ingredients": "155:*:1|155:*:1|155:*:1|155:*:1|155:*:1|155:*:1" },
  { "recipe": "77", "result": "298:0:1", "ingredients": "334:*:1|334:*:1|334:*:1|334:*:1|334:*:1" },
  { "recipe": "78", "result": "299:0:1", "ingredients": "334:*:1|334:*:1|334:*:1|334:*:1|334:*:1|334:*:1|334:*:1|334:*:1" },
  { "recipe": "79", "result": "300:0:1", "ingredients": "334:*:1|334:*:1|334:*:1|334:*:1|334:*:1|334:*:1|334:*:1" },
  { "recipe": "80", "result": "301:0:1", "ingredients": "334:*:1|334:*:1|334:*:1|334:*:1" },
  { "recipe": "81", "result": "306:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "82", "result": "307:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "83", "result": "308:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "84", "result": "309:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "85", "result": "310:0:1", "ingredients": "264:*:1|264:*:1|264:*:1|264:*:1|264:*:1" },
  { "recipe": "86", "result": "311:0:1", "ingredients": "264:*:1|264:*:1|264:*:1|264:*:1|264:*:1|264:*:1|264:*:1|264:*:1" },
  { "recipe": "87", "result": "312:0:1", "ingredients": "264:*:1|264:*:1|264:*:1|264:*:1|264:*:1|264:*:1|264:*:1" },
  { "recipe": "88", "result": "313:0:1", "ingredients": "264:*:1|264:*:1|264:*:1|264:*:1" },
  { "recipe": "89", "result": "314:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|266:*:1|266:*:1" },
  { "recipe": "90", "result": "315:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|266:*:1|266:*:1|266:*:1|266:*:1|266:*:1" },
  { "recipe": "91", "result": "316:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|266:*:1|266:*:1|266:*:1|266:*:1" },
  { "recipe": "92", "result": "317:0:1", "ingredients": "266:*:1|266:*:1|266:*:1|266:*:1" }
)JSON";

        // Recipes 93-124 in original LCE registration order.
        inline constexpr const char* kWoolAndClay = R"JSON(
  { "recipe": "93", "result": "35:0:1", "ingredients": "351:0:1|35:0:1" },
  { "recipe": "94", "result": "159:0:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:0:1" },
  { "recipe": "95", "result": "35:1:1", "ingredients": "351:1:1|35:0:1" },
  { "recipe": "96", "result": "159:1:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:1:1" },
  { "recipe": "97", "result": "35:2:1", "ingredients": "351:2:1|35:0:1" },
  { "recipe": "98", "result": "159:2:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:2:1" },
  { "recipe": "99", "result": "35:3:1", "ingredients": "351:3:1|35:0:1" },
  { "recipe": "100", "result": "159:3:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:3:1" },
  { "recipe": "101", "result": "35:4:1", "ingredients": "351:4:1|35:0:1" },
  { "recipe": "102", "result": "159:4:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:4:1" },
  { "recipe": "103", "result": "35:5:1", "ingredients": "351:5:1|35:0:1" },
  { "recipe": "104", "result": "159:5:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:5:1" },
  { "recipe": "105", "result": "35:6:1", "ingredients": "351:6:1|35:0:1" },
  { "recipe": "106", "result": "159:6:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:6:1" },
  { "recipe": "107", "result": "35:7:1", "ingredients": "351:7:1|35:0:1" },
  { "recipe": "108", "result": "159:7:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:7:1" },
  { "recipe": "109", "result": "35:8:1", "ingredients": "351:8:1|35:0:1" },
  { "recipe": "110", "result": "159:8:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:8:1" },
  { "recipe": "111", "result": "35:9:1", "ingredients": "351:9:1|35:0:1" },
  { "recipe": "112", "result": "159:9:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:9:1" },
  { "recipe": "113", "result": "35:10:1", "ingredients": "351:10:1|35:0:1" },
  { "recipe": "114", "result": "159:10:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:10:1" },
  { "recipe": "115", "result": "35:11:1", "ingredients": "351:11:1|35:0:1" },
  { "recipe": "116", "result": "159:11:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:11:1" },
  { "recipe": "117", "result": "35:12:1", "ingredients": "351:12:1|35:0:1" },
  { "recipe": "118", "result": "159:12:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:12:1" },
  { "recipe": "119", "result": "35:13:1", "ingredients": "351:13:1|35:0:1" },
  { "recipe": "120", "result": "159:13:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:13:1" },
  { "recipe": "121", "result": "35:14:1", "ingredients": "351:14:1|35:0:1" },
  { "recipe": "122", "result": "159:14:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:14:1" },
  { "recipe": "123", "result": "35:15:1", "ingredients": "351:15:1|35:0:1" },
  { "recipe": "124", "result": "159:15:8", "ingredients": "172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|172:0:1|351:15:1" }
)JSON";

        // Recipes 125-155 in original LCE registration order.
        inline constexpr const char* kDyesAndCarpet = R"JSON(
  { "recipe": "125", "result": "351:11:2", "ingredients": "37:*:1" },
  { "recipe": "126", "result": "351:1:2", "ingredients": "38:*:1" },
  { "recipe": "127", "result": "351:15:3", "ingredients": "352:*:1" },
  { "recipe": "128", "result": "351:9:2", "ingredients": "351:1:1|351:15:1" },
  { "recipe": "129", "result": "351:14:2", "ingredients": "351:1:1|351:11:1" },
  { "recipe": "130", "result": "351:10:2", "ingredients": "351:2:1|351:15:1" },
  { "recipe": "131", "result": "351:8:2", "ingredients": "351:0:1|351:15:1" },
  { "recipe": "132", "result": "351:7:2", "ingredients": "351:8:1|351:15:1" },
  { "recipe": "133", "result": "351:7:3", "ingredients": "351:0:1|351:15:1|351:15:1" },
  { "recipe": "134", "result": "351:12:2", "ingredients": "351:4:1|351:15:1" },
  { "recipe": "135", "result": "351:6:2", "ingredients": "351:4:1|351:2:1" },
  { "recipe": "136", "result": "351:5:2", "ingredients": "351:4:1|351:1:1" },
  { "recipe": "137", "result": "351:13:2", "ingredients": "351:5:1|351:9:1" },
  { "recipe": "138", "result": "351:13:3", "ingredients": "351:4:1|351:1:1|351:9:1" },
  { "recipe": "139", "result": "351:13:4", "ingredients": "351:4:1|351:1:1|351:1:1|351:15:1" },
  { "recipe": "140", "result": "171:0:3", "ingredients": "35:0:1|35:0:1" },
  { "recipe": "141", "result": "171:1:3", "ingredients": "35:1:1|35:1:1" },
  { "recipe": "142", "result": "171:2:3", "ingredients": "35:2:1|35:2:1" },
  { "recipe": "143", "result": "171:3:3", "ingredients": "35:3:1|35:3:1" },
  { "recipe": "144", "result": "171:4:3", "ingredients": "35:4:1|35:4:1" },
  { "recipe": "145", "result": "171:5:3", "ingredients": "35:5:1|35:5:1" },
  { "recipe": "146", "result": "171:6:3", "ingredients": "35:6:1|35:6:1" },
  { "recipe": "147", "result": "171:7:3", "ingredients": "35:7:1|35:7:1" },
  { "recipe": "148", "result": "171:8:3", "ingredients": "35:8:1|35:8:1" },
  { "recipe": "149", "result": "171:9:3", "ingredients": "35:9:1|35:9:1" },
  { "recipe": "150", "result": "171:10:3", "ingredients": "35:10:1|35:10:1" },
  { "recipe": "151", "result": "171:11:3", "ingredients": "35:11:1|35:11:1" },
  { "recipe": "152", "result": "171:12:3", "ingredients": "35:12:1|35:12:1" },
  { "recipe": "153", "result": "171:13:3", "ingredients": "35:13:1|35:13:1" },
  { "recipe": "154", "result": "171:14:3", "ingredients": "35:14:1|35:14:1" },
  { "recipe": "155", "result": "171:15:3", "ingredients": "35:15:1|35:15:1" }
)JSON";

        // Recipes 156-199 in original LCE registration order.
        inline constexpr const char* kMaterialsAndTransport = R"JSON(
  { "recipe": "156", "result": "80:0:1", "ingredients": "332:*:1|332:*:1|332:*:1|332:*:1" },
  { "recipe": "157", "result": "78:0:6", "ingredients": "80:*:1|80:*:1|80:*:1" },
  { "recipe": "158", "result": "82:0:1", "ingredients": "337:*:1|337:*:1|337:*:1|337:*:1" },
  { "recipe": "159", "result": "45:0:1", "ingredients": "336:*:1|336:*:1|336:*:1|336:*:1" },
  { "recipe": "160", "result": "35:0:1", "ingredients": "287:*:1|287:*:1|287:*:1|287:*:1" },
  { "recipe": "161", "result": "46:0:1", "ingredients": "289:*:1|12:*:1|289:*:1|12:*:1|289:*:1|12:*:1|289:*:1|12:*:1|289:*:1" },
  { "recipe": "162", "result": "44:1:6", "ingredients": "24:*:1|24:*:1|24:*:1" },
  { "recipe": "163", "result": "44:0:6", "ingredients": "1:*:1|1:*:1|1:*:1" },
  { "recipe": "164", "result": "44:3:6", "ingredients": "4:*:1|4:*:1|4:*:1" },
  { "recipe": "165", "result": "44:4:6", "ingredients": "45:*:1|45:*:1|45:*:1" },
  { "recipe": "166", "result": "44:5:6", "ingredients": "98:*:1|98:*:1|98:*:1" },
  { "recipe": "167", "result": "44:6:6", "ingredients": "112:*:1|112:*:1|112:*:1" },
  { "recipe": "168", "result": "44:7:6", "ingredients": "155:*:1|155:*:1|155:*:1" },
  { "recipe": "169", "result": "126:0:6", "ingredients": "5:0:1|5:0:1|5:0:1" },
  { "recipe": "170", "result": "126:2:6", "ingredients": "5:2:1|5:2:1|5:2:1" },
  { "recipe": "171", "result": "126:1:6", "ingredients": "5:1:1|5:1:1|5:1:1" },
  { "recipe": "172", "result": "126:3:6", "ingredients": "5:3:1|5:3:1|5:3:1" },
  { "recipe": "173", "result": "354:0:1", "ingredients": "335:*:1|335:*:1|335:*:1|353:*:1|344:*:1|353:*:1|296:*:1|296:*:1|296:*:1" },
  { "recipe": "174", "result": "353:0:1", "ingredients": "338:*:1" },
  { "recipe": "175", "result": "66:0:16", "ingredients": "265:*:1|265:*:1|265:*:1|280:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "176", "result": "27:0:6", "ingredients": "266:*:1|266:*:1|266:*:1|280:*:1|266:*:1|266:*:1|331:*:1|266:*:1" },
  { "recipe": "177", "result": "157:0:6", "ingredients": "265:*:1|280:*:1|265:*:1|265:*:1|76:*:1|265:*:1|265:*:1|280:*:1|265:*:1" },
  { "recipe": "178", "result": "28:0:6", "ingredients": "265:*:1|265:*:1|265:*:1|70:*:1|265:*:1|265:*:1|331:*:1|265:*:1" },
  { "recipe": "179", "result": "328:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "180", "result": "342:0:1", "ingredients": "54:*:1|328:*:1" },
  { "recipe": "181", "result": "343:0:1", "ingredients": "61:*:1|328:*:1" },
  { "recipe": "182", "result": "407:0:1", "ingredients": "46:*:1|328:*:1" },
  { "recipe": "183", "result": "408:0:1", "ingredients": "154:*:1|328:*:1" },
  { "recipe": "184", "result": "333:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "185", "result": "346:0:1", "ingredients": "280:*:1|280:*:1|287:*:1|280:*:1|287:*:1" },
  { "recipe": "186", "result": "398:0:1", "ingredients": "346:*:1|391:*:1" },
  { "recipe": "187", "result": "259:0:1", "ingredients": "265:*:1|318:*:1" },
  { "recipe": "188", "result": "297:0:1", "ingredients": "296:*:1|296:*:1|296:*:1" },
  { "recipe": "189", "result": "261:0:1", "ingredients": "280:*:1|287:*:1|280:*:1|287:*:1|280:*:1|287:*:1" },
  { "recipe": "190", "result": "262:0:4", "ingredients": "318:*:1|280:*:1|288:*:1" },
  { "recipe": "191", "result": "268:0:1", "ingredients": "5:*:1|5:*:1|280:*:1" },
  { "recipe": "192", "result": "272:0:1", "ingredients": "4:*:1|4:*:1|280:*:1" },
  { "recipe": "193", "result": "267:0:1", "ingredients": "265:*:1|265:*:1|280:*:1" },
  { "recipe": "194", "result": "276:0:1", "ingredients": "264:*:1|264:*:1|280:*:1" },
  { "recipe": "195", "result": "283:0:1", "ingredients": "266:*:1|266:*:1|280:*:1" },
  { "recipe": "196", "result": "325:0:1", "ingredients": "265:*:1|265:*:1|265:*:1" },
  { "recipe": "197", "result": "281:0:4", "ingredients": "5:*:1|5:*:1|5:*:1" },
  { "recipe": "198", "result": "374:0:3", "ingredients": "20:*:1|20:*:1|20:*:1" },
  { "recipe": "199", "result": "390:0:1", "ingredients": "336:*:1|336:*:1|336:*:1" }
)JSON";

        // Recipes 200-236 in original LCE registration order.
        inline constexpr const char* kMechanisms = R"JSON(
  { "recipe": "200", "result": "50:0:4", "ingredients": "263:1:1|280:*:1" },
  { "recipe": "201", "result": "50:0:4", "ingredients": "263:0:1|280:*:1" },
  { "recipe": "202", "result": "89:0:1", "ingredients": "348:*:1|348:*:1|348:*:1|348:*:1" },
  { "recipe": "203", "result": "155:0:1", "ingredients": "406:*:1|406:*:1|406:*:1|406:*:1" },
  { "recipe": "204", "result": "69:0:1", "ingredients": "280:*:1|4:*:1" },
  { "recipe": "205", "result": "131:0:2", "ingredients": "265:*:1|280:*:1|5:*:1" },
  { "recipe": "206", "result": "76:0:1", "ingredients": "331:*:1|280:*:1" },
  { "recipe": "207", "result": "356:0:1", "ingredients": "76:*:1|331:*:1|76:*:1|1:*:1|1:*:1|1:*:1" },
  { "recipe": "208", "result": "404:0:1", "ingredients": "76:*:1|76:*:1|406:*:1|76:*:1|1:*:1|1:*:1|1:*:1" },
  { "recipe": "209", "result": "151:0:1", "ingredients": "20:*:1|20:*:1|20:*:1|406:*:1|406:*:1|406:*:1|126:*:1|126:*:1|126:*:1" },
  { "recipe": "210", "result": "154:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|54:*:1|265:*:1|265:*:1" },
  { "recipe": "211", "result": "347:0:1", "ingredients": "266:*:1|266:*:1|331:*:1|266:*:1|266:*:1" },
  { "recipe": "212", "result": "381:0:1", "ingredients": "368:*:1|377:*:1" },
  { "recipe": "213", "result": "385:0:3", "ingredients": "289:*:1|377:*:1|263:*:1" },
  { "recipe": "214", "result": "385:0:3", "ingredients": "289:*:1|377:*:1|263:1:1" },
  { "recipe": "215", "result": "420:0:2", "ingredients": "287:*:1|287:*:1|287:*:1|341:*:1|287:*:1" },
  { "recipe": "216", "result": "345:0:1", "ingredients": "265:*:1|265:*:1|331:*:1|265:*:1|265:*:1" },
  { "recipe": "217", "result": "358:0:1", "ingredients": "339:*:1|339:*:1|339:*:1|339:*:1|345:*:1|339:*:1|339:*:1|339:*:1|339:*:1" },
  { "recipe": "218", "result": "77:0:1", "ingredients": "1:*:1" },
  { "recipe": "219", "result": "143:0:1", "ingredients": "5:*:1" },
  { "recipe": "220", "result": "72:0:1", "ingredients": "5:*:1|5:*:1" },
  { "recipe": "221", "result": "70:0:1", "ingredients": "1:*:1|1:*:1" },
  { "recipe": "222", "result": "148:0:1", "ingredients": "265:*:1|265:*:1" },
  { "recipe": "223", "result": "147:0:1", "ingredients": "266:*:1|266:*:1" },
  { "recipe": "224", "result": "23:0:1", "ingredients": "4:*:1|4:*:1|4:*:1|4:*:1|261:*:1|4:*:1|4:*:1|331:*:1|4:*:1" },
  { "recipe": "225", "result": "158:0:1", "ingredients": "4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|4:*:1|331:*:1|4:*:1" },
  { "recipe": "226", "result": "380:0:1", "ingredients": "265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1|265:*:1" },
  { "recipe": "227", "result": "379:0:1", "ingredients": "369:*:1|4:*:1|4:*:1|4:*:1" },
  { "recipe": "228", "result": "91:0:1", "ingredients": "86:*:1|50:*:1" },
  { "recipe": "229", "result": "84:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|264:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "230", "result": "339:0:3", "ingredients": "338:*:1|338:*:1|338:*:1" },
  { "recipe": "231", "result": "340:0:1", "ingredients": "339:*:1|339:*:1|339:*:1|334:*:1" },
  { "recipe": "232", "result": "25:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|331:*:1|5:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "233", "result": "47:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|340:*:1|340:*:1|340:*:1|5:*:1|5:*:1|5:*:1" },
  { "recipe": "234", "result": "321:0:1", "ingredients": "280:*:1|280:*:1|280:*:1|280:*:1|35:*:1|280:*:1|280:*:1|280:*:1|280:*:1" },
  { "recipe": "235", "result": "389:0:1", "ingredients": "280:*:1|280:*:1|280:*:1|280:*:1|334:*:1|280:*:1|280:*:1|280:*:1|280:*:1" },
  { "recipe": "236", "result": "41:0:1", "ingredients": "266:0:1|266:0:1|266:0:1|266:0:1|266:0:1|266:0:1|266:0:1|266:0:1|266:0:1" }
)JSON";

        // Recipes 237-252 in original LCE registration order.
        inline constexpr const char* kOreAndCompression = R"JSON(
  { "recipe": "237", "result": "266:0:9", "ingredients": "41:0:1" },
  { "recipe": "238", "result": "42:0:1", "ingredients": "265:0:1|265:0:1|265:0:1|265:0:1|265:0:1|265:0:1|265:0:1|265:0:1|265:0:1" },
  { "recipe": "239", "result": "265:0:9", "ingredients": "42:0:1" },
  { "recipe": "240", "result": "57:0:1", "ingredients": "264:0:1|264:0:1|264:0:1|264:0:1|264:0:1|264:0:1|264:0:1|264:0:1|264:0:1" },
  { "recipe": "241", "result": "264:0:9", "ingredients": "57:0:1" },
  { "recipe": "242", "result": "133:0:1", "ingredients": "388:0:1|388:0:1|388:0:1|388:0:1|388:0:1|388:0:1|388:0:1|388:0:1|388:0:1" },
  { "recipe": "243", "result": "388:0:9", "ingredients": "133:0:1" },
  { "recipe": "244", "result": "22:0:1", "ingredients": "351:4:1|351:4:1|351:4:1|351:4:1|351:4:1|351:4:1|351:4:1|351:4:1|351:4:1" },
  { "recipe": "245", "result": "351:4:9", "ingredients": "22:0:1" },
  { "recipe": "246", "result": "152:0:1", "ingredients": "331:0:1|331:0:1|331:0:1|331:0:1|331:0:1|331:0:1|331:0:1|331:0:1|331:0:1" },
  { "recipe": "247", "result": "331:0:9", "ingredients": "152:0:1" },
  { "recipe": "248", "result": "173:0:1", "ingredients": "263:0:1|263:0:1|263:0:1|263:0:1|263:0:1|263:0:1|263:0:1|263:0:1|263:0:1" },
  { "recipe": "249", "result": "263:0:9", "ingredients": "173:0:1" },
  { "recipe": "250", "result": "170:0:1", "ingredients": "296:0:1|296:0:1|296:0:1|296:0:1|296:0:1|296:0:1|296:0:1|296:0:1|296:0:1" },
  { "recipe": "251", "result": "296:0:9", "ingredients": "170:0:1" },
  { "recipe": "252", "result": "266:0:1", "ingredients": "371:*:1|371:*:1|371:*:1|371:*:1|371:*:1|371:*:1|371:*:1|371:*:1|371:*:1" }
)JSON";

        // Recipes 253-259 in original LCE registration order.
        inline constexpr const char* kLateWorkbench = R"JSON(
  { "recipe": "253", "result": "371:0:9", "ingredients": "266:*:1" },
  { "recipe": "254", "result": "323:0:3", "ingredients": "5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|5:*:1|280:*:1" },
  { "recipe": "255", "result": "33:0:1", "ingredients": "5:*:1|5:*:1|5:*:1|4:*:1|265:*:1|4:*:1|4:*:1|331:*:1|4:*:1" },
  { "recipe": "256", "result": "29:0:1", "ingredients": "341:*:1|33:*:1" },
  { "recipe": "257", "result": "401:0:1", "ingredients": "339:*:1|289:*:1" },
  { "recipe": "258", "result": "402:0:1", "ingredients": "351:*:1|289:*:1" },
  { "recipe": "259", "result": "402:0:1", "ingredients": "351:*:1|402:*:1" }
)JSON";

        inline std::string BuildInventoryRecipesJson()
        {
            const char* sections[] =
            {
                kCoreAndTools,
                kFoodAndStructures,
                kWorkbenchBasics,
                kWoolAndClay,
                kDyesAndCarpet,
                kMaterialsAndTransport,
                kMechanisms,
                kOreAndCompression,
                kLateWorkbench,
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
