unsigned long lastSecond = 0;
unsigned long lastBlink = 0;
unsigned long timeRemaining = 40000; // ms
unsigned int state = 0;
bool ledState = false;

void setup() {
  pinMode(25, OUTPUT); // LED 1
}

void loop() {
  unsigned long now = millis();

  if (now - lastSecond >= 1000 && timeRemaining > 0) {
    lastSecond = now;
    timeRemaining -= 1000;

    Serial.print(timeRemaining / 1000);
    Serial.print("\n");
  }

  if (timeRemaining == 0) {
    digitalWrite(25, 1);
    sleep_ms(1000);
    if (state == 0) {
      state = 1;
      Serial.print(
      "                                                                                                                                  \n"
      "                                                                                                                                  \n"
      "                                                    ..:^^~!!777777777!!~^^:..                                                    \n"
      "                                             :^!7JY5PGGBBBBBBBBBBBBBBBBBBGGGP5Y?7~:.                                             \n"
      "                                       .:~?YPGBBBBBBBBGGGGGPPPGBGPGGGPPGBBGGBBBBBBGG5J7~:                                        \n"
      "                                   .^7YPGBBBBBBGGGGPPP55555JJJJ5PP55PJJPGGGPYYPGGBBBBBBBG5J!:                                    \n"
      "                                :!YPBBBBBBBBGPYJ5G5YYY5PPP5JJJJJJJJYYJ5GGBB5JJ5GBBBGGGBBBBBBGPJ~.                                \n"
      "                             :75GBBBBBGGPPGGPJ^^YYJJJYGBG5JY5PP5YJJJJ775PY75JJPGGBBGPPGGBBBBBBBBGY!.                             \n"
      "                          .!5GBBBBBBGBG5JJY5PGPJJJJJ5GGPJ5PGBBBBGPYJJ?  . :YJJY555555YY5PGGGP55GBBBGJ~.                          \n"
      "                        ^JGBBBBGGGBGBGGPYJJJJYJJJJJJ55YY5GGGP55PGBGP7:.   .^7JJJJJJYYJJJJYYYJJJJY5GBBBP7:                        \n"
      "                      ~5BBBGGBBG55GGBG555YJJJJJJJJYYJJYGGYJJJJJPGGBB5JJ~ 7Y5P55YY5GGG5?JJJJJJJJYYJJY5GBBGJ:                      \n"
      "                    ~5BBBGGGGGBG5JY555YJJJJJYY5JJJYGGYYGPYJJJJJ5GGGPJJJJ?YGBP5PGGGBGGG5GPYJJJ5GGGYJJY5PGBBGJ:                 .:^\n"
      "                  ^5BBBGGBBBBBBBP5YYYYYY55PGGBBPJJJJ5PY5GP5YJJYPGGG5JJJJJPGGP5PBGGGGGP5PGYJJJ5G5JYPGGGGPGBBBGJ.           .^!7JYYY\n"
      "                .JBBBGBBBGP5J?7!!!~~~!7?JJYY5PGGP5JJJY55GGGG55GGGBPYJJJJJPGGG5J5GGGBGYJ55JJJJ5GGPGBBG5JJ5GGGBBG!     .^~7JYYYYYYYY\n"
      "               ~PBBGBBGY!:..^!???JJJ5PGP5P555J???J?7??J5G5PBGBBBG5JJJ??YPGGBG5JJPBPJYYJ77JJJJYPPPGBGGGP5~~5PPGBBY^~7?JYYYYYYYYYYYY\n"
      "              ?BBGGBBJ:  ^YGBGP5YY5PGBBGYGGGPP55JJ55J777?Y555PG5JJJJJ?JGBGBBGYY5PPY: .. J5YJJJJPGP5PGBBGYJPYJY55YYYYYYYYJ??JYYYYYY\n"
      "            .5BBGGBG!   ?BBGPPYJY5PP55GBBGGGGGBG5JYJYJJ7!??777???JJJJJJ5PPGG5JYYJ?~.   .~J5JJJJYGPJJ55PP55YYYYYYYYYYJ7~^~7JYYYYYJY\n"
      "           :PBGGGBB?   !P555YYJJY5555YPGGBG55PGGG5J~ :. 7JJJJJ??777?YYJJJYYJJJJJJJYPJ ^JY5Y5YJJJJYY5YYYYYYYYYYYY55PGJ:!?YYYYYY?~.\n"
      "          :PBGGGGBB^   YP5JJJ5YJYPGBG5J5GGG55GGBGJ~:    :!JJJJJJJJ?J?77??JJJYJYPPGBBGJJJYGBG5JJJYYYYYYYYYYYYYPGGBGGP5YYYYYY?!:   \n"
      "         :PBGGGGBGB!   YBPJJJ5555Y5GBPJJ5GBGGGGPYJJ?7.:???JP5YJJYYYPPY???7??PPYYPGGP55555P5YYYYYYYYYYYYJJYPGP5GGPYYYYYYYJ!:      \n"
      "        .5BGGGGGBGG5   ~BGYJJJJYYJJPG55YJ5P55YJJY5YJJ7?JJY55YYPGPYJ7YYJJ5J?J555JJ5PY555YYYYYYYYYYYYY5GYJJYGG5JYYYYYYYY5J         \n"
      "        ?BGGGGBBBBGB?   ?GYJJJJYPYJJJJJ?~~?JJJJJJYYJJJYPYY5YJJPPJJJ?JJJJJJJ55Y??7?YYYYYYYYYYY555PYJ5GBPJJJ55YYYYYYY5PGBG~        \n"
      "       ~BBGGGBGGGGGGG~   ?JJJJJJYYJJJJJ?~~JYY5YY55YJJJJYJJYGGGGYJJJJYYJJJJJJJYYYYJ??JYYYYJY5GBBGP!:?5GG5JYYYYYYYY5PGGGBBP.       \n"
      "       5BGGBBGPPGGGPP5~   !JJJJJJJJJJJJJJJJJYPPPPPP555JJJJJY5PYJJYYPP5YJYYYYYYYYYYYY????JJPGGGGPYJJYYYYYYYYYYYY5PPGP5PGBB7       \n"
      "      ~BBGBBG55PGBBGGGP!   ~JJJJJJ7!~^^^~7JJJJ5GGGPPGBPJ?~~~~!JJ5P5YYYYYYYYYYYYJJ7~~~~!?77JYP5Y5J77!!7YYYYYJJ5PPP5YJJ5GBBP.      \n"
      "      YBGGBGP55GGP55GPY5?.  :?YYY:        ^?JJJJYYJJPG5J!    .JYYYYYYYYYYYYJJJJJJ^    ~PYJ?7?JJY~    ^YYJJJJPGGGG5YYY5GBBB!      \n"
      "     .GBGBG5YY5PGGPPGGPPGJ:   !5^    ..    ^JY5YJJJJJJJY7    .YYYYYY555JYPGGPJJJJ^    ~BGGPY?7?Y~    ~PYJJJYGBJ7YGP?~PBBGBY      \n"
      "     ~BBGBPYY5GGGBBBBBBG5YY?:  .    .PY     7GYJJJJJYYYY7    .JYJYGGP5JJYPP5YJJJJ^    ^P5YYYYY?7^    ~5JJJJ5GBP  .. ~BBBGBP.     \n"
      "     7BGGBP5Y55PPPP55GG5YYY55!     .YBG7     ?5YYYYYYYYY7    .5YJJPG5JJYYYYYYYJJJ^    ^YYYYYYYY5~    ^JJJJJ5GJ~     .!5GBBG:     \n"
      "     ?BGGBGP5YYYY555PGGP5YYYP?     :JPGP~    .?YYYYYYY5PJ    .55YYYGGGGGGGGBBPJJJ^    ^YYYYY5PGB!    ^JJJJJPPJ?J~  7JJYGBBB^     \n"
      "     ?BGGBGGGGG555PGBGP55YYY?.       :7YY:    :J55PYJJPBY    .JPPJJGGGY5GGBBBG5JJ^    ^YY5PGGGGP!    :7555Y5PPGGG^7BBBBBGBB^     \n"
      "     7BGGGBBBBG5YPGPPGP5YY5Y.    ^J~.  :!?.    ~GBP5YJJ5J    .JGPJ5GBG5PGBBBGG5YY~    ^YY5PP5YJJ~    ^!7PG55555PGGGBGGGGGBG:     \n"
      "     ~BBGGGGGBGGPGBGGG55YYY:    :JYYJ!.  .:     7GGGBPJJ?     ~YYJ5GPGPPGGP5YYYYY~    .7J55555Y7.    ~Y?^!5GP5PPGBBBBGGGGBP.     \n"
      "     :GBGGBBBBBBBGGP5YYYYY^    .J5YYJJJ7^        7GGG5JJP?      ..........:YYYYYYJ.     .......     :5PYJ~~PGPGGGBB55BGGGBY      \n"
      "      5BBBBGGPP55YYYYYYYY~     YBGP555YJJ?~:     .5G5JYPGP7^.             .YYYYJJJ?~.             .~YGYJYY7:5BBBBGBPGBGGGB7      \n"
      "      !GPP5YYYYYYYYYYYY55?77!~?GBPJ5GBBPJJJJ?^.   :??YPPYJJJ?JJ?\?!!!!!!!!!!JJJJJJJJJJ?7!~~~!!!7777JJ55JJJYP?.YGBBBBBBGGGBG:      \n"
      "      .JYYYYYYYYY55P5JYGGBPJJJJ5GPY5PBGPJJJJJJJ7^.  .^!?JJJJ5GGP5YYYYYYY5PPYJJJJJJJJJJJJJJJJJJYYJJJJYYY5GGGB! YPGGBBBGGGBJ       \n"
      "       ~YYYY5PPGGBBG5J5GGGG5YJYPGG5~75YJJJJJJJYPGPY!.   :~7JP5YYYYYYY5PGGGBPJJ???J5YJJ555555JJJJJJJJJJY5PPPGP.:5YJ7J?BBBG:       \n"
      "        JGGGBBBBBBBGP5GBP?~5GGGBGBGYJJJJJY5YJJJYPGBGJ7~.   .^7JYYYYY5P5YJYYYJ?: ~5GGPYYGBGPJ?YYJJJJJJJJJJJJJY^ !Y?: ~5BB!        \n"
      "        :GBBBGGGGGGBGGBBPJ7PBBBGGGYJJJ?~JJ!JYYJJYJJJYYYY?!^.   :~7YPGGP5YYYJJJ?!YGY5GG55GGY^7GGY77JY5YJJY5PP5^ .JYY?GGBJ         \n"
      "         ^GBGGGGGGGGBGGG5PGGGP55YYJJJY?   .JPG5JJJJY5YYYYYYJJ7^.   :~7YPBBGYJJJJPGGGPYYPP55GBBPY??YGGGPYJYGBY  :5YY5GB5.         \n"
      "          ~GBGGGGGGBBG5YYYYYJJJJJJJ5P5J?.^JJGBGYJYYYYYYYY5PG55PP?~:.   .:~!7?JJJY5GPYJ55JJ5GBGYJJYPGGBG5YYPY.  !BGGGBP.          \n"
      "           ~GBGGGGGBGPYJ!7!JJJJJY5GG5YP5J5PPPPYYYYYYYYYJ5GGGJ75G5YJJ?\?!^.    .:^!7?\?\?YGPY5PPPYJJYPGGGBB5?!^   :Y5GBB5:           \n"
      "            ^PBGGGGBGG57. :?JJJJY5PB5J5PPGP5YYYYYYYYJJJJJJY5J~?5GGPPGBBGY7!~:.      .:~!~~!7777?YJ77!!~:    .!Y5PGBY.            \n"
      "             :5BBGGGBBGPP?PYJJJJJJJ5YYY55YYYYYYYYJJJJJJJJJJPGPYY5GGPPP5YJJJJJJ?777~^:.                  .:~?Y5PGBB?              \n"
      "               ?GBBGGBGGBBB5JJPGP5JJJJYYYYYYYYJJJJJJJJ55JJJJY55YJYJJJJJJJJY5PYJJJ5GGP555YJ7!~!!!~~^~~!?YYPGP5PGBP~               \n"
      "                ^5BBGBGGPGPYJ5BGGPYYYYYYYYY5GGPY7?JYJJGGYJYJJJJJJJJY555YJJPBBGYJJJ5GG55GGP55PBBBBGG5J5PYPPYYPBBJ.                \n"
      "                  7PBBBGGGYJJYP5YYYYYYYYY5PGBBBG^.^^.?GPPGGGYJJJJYJ5BG5JJ5GGG5JJJJY5555P555Y5GGBBBG5JYJ5P5PBB5^                  \n"
      "                   .?GBBBGPYYYYYYYYYYPPPPGGBBBP?:    ~J5PGGPJJJJJYGPP5JJ5GPYYJJJJJYY5PP5PGP5J55PBGPJJJ5GGBBP!                    \n"
      "                     :?PG5YYYYYYYYYGGGBBBBBBGPJ77! .JJYP5JYYJJJJJJJJJJJJJJJJY5YJJP5Y5GG5PGBBGP5Y55JJJ5GBB5!                      \n"
      "                       .~?YYYY5PGGGYYGGBBGGBGPPPPG?JBGPGG55GGYJJJJJJY5YYYPGGJJ?55PG55PP55PGGBBGPJJY5GBGY~                        \n"
      "                          ^75GBBBBBYJPGGBGGGBGGGGBBBGG5PGBBG5J?^~JYPGGG5JYGP! ~PGPPPPPPP5Y^!GBGGPPBBP?:                          \n"
      "                            .!YGBBBGPGBBGGBBGBBBBBBBBBGPPBBG5YJ!7YPGPPPPJJPGG5GGGGPPPPPGG5JYGBBBBPJ^                             \n"
      "                               .~JPBBBBBBBPPBGGGGGGGBBGGGGGP55555GBBBGGG5J5GGGGGGP55P555GBGBBG5?^.                               \n"
      "                                  .^7YPGBBBBBBGGGGGBGGGP55555555PGGGGBBGP5P5PPPPGGGGGBBBBG5J!:                                   \n"
      "                                      .:!?YPGBBBBBBBGP5555555555PPPPPGGGGBGPGGBBBBBBGPY7~:                                       \n"
      "                                           .:~7?Y5PGGBBBBBBBBGGGGBBBGBBBBBBBGGP5J?!^:                                            \n"
      "                                                  .:^^~!7?JJJYYYYYYYJJ??7!~^:..                                                  \n"
      "                                                                                                                                  \n"
      "                                                                                                                                  \n"
      );
    }
  }

  unsigned int blinkSpeed = map(timeRemaining, 0, 40000, 50, 500);
  if (now - lastBlink >= blinkSpeed) {
    lastBlink = now;

    ledState = !ledState;
    digitalWrite(25, ledState);
  }
}