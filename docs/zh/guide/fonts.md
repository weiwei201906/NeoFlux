# 瀛椾綋绯荤粺

NeoFlux 浣跨敤 FreeType 杩涜瀛椾綋鍏夋爡鍖栵紝骞朵娇鐢ㄥ瓧褰㈢汗鐞嗗浘闆嗗疄鐜伴珮鏁堟枃鏈覆鏌撱€?

## 閰嶇疆瀛椾綋鐩綍

:::danger
蹇呴』鍦ㄨ皟鐢?`Application::Init()` 涔嬪墠閰嶇疆瀛椾綋鐩綍銆傚鏋滄壘涓嶅埌瀛椾綋锛屾墍鏈夋枃鏈?Widget 閮戒細娓叉煋涓轰贡鐮佹垨绌虹櫧銆?
:::

榛樿鎯呭喌涓嬶紝NeoFlux 鎵弿 `fonts/` 鐩綍鏌ユ壘瀛椾綋鏂囦欢銆傚闇€浣跨敤鍏朵粬鐩綍锛屽湪 `Init()` **涔嬪墠**璋冪敤 `Application::SetFontDir()`锛?

```cpp
Application app;
app.SetFontDir("./fonts/");  // 鎵弿 assets/fonts/ 鑰岄潪 fonts/
app.Init(argc, argv, 800, 600, "My App");
app.PushRoute("/");
app.Run();
```

`SetFontDir()` 鎺ュ彈鐩稿鎴栫粷瀵硅矾寰勩€傜浉瀵硅矾寰勪粠宸ヤ綔鐩綍瑙ｆ瀽锛屽苟鑷姩鍚戜笂鍥為€€锛坄../`銆乣../../`锛変互閫傞厤鏋勫缓瀛愮洰褰曘€?

## 娣诲姞瀛椾綋

灏嗗瓧浣撴枃浠讹紙`.ttf`銆乣.otf`銆乣.ttc`锛夋斁鍏ラ厤缃殑瀛椾綋鐩綍锛?

```
fonts/                    锛堥粯璁わ紝鎴栦綘鑷畾涔夌殑璺緞锛?
  NotoSansSC-Regular.ttf
  Roboto-Bold.ttf
```

`FontManager` 鍦ㄥ惎鍔ㄦ椂鎵弿姝ょ洰褰曪紝鎸夋枃浠跺悕锛堜笉鍚墿灞曞悕锛夋敞鍐屽瓧浣撱€?

:::warning
濡傛灉閰嶇疆鐨勫瓧浣撶洰褰曚负绌烘垨涓嶅瓨鍦紝鎵€鏈夋枃鏈?Widget 閮戒細娓叉煋寮傚父锛堜贡鐮佹垨绌虹櫧锛夈€傚彂甯冨簲鐢ㄦ椂鍔″繀鑷冲皯闄勫甫涓€涓瓧浣撴枃浠躲€傛覆鏌撲腑鏂囬渶鍖呭惈 CJK 瀛椾綋锛堝 NotoSansSC锛夈€?
:::

## 鏀寔鏍煎紡

- TrueType锛坄.ttf`锛?
- OpenType锛坄.otf`锛?
- TrueType Collection锛坄.ttc`锛?

## 浣跨敤瀛椾綋

Widget 閫氳繃鏂囦欢鍚嶏紙涓嶅惈鎵╁睍鍚嶏級寮曠敤瀛椾綋锛?

```cpp
auto text = std::make_shared<Text>("Hello World");
text->SetFont("NotoSansSC-Regular");  // 鍔犺浇閰嶇疆鐩綍涓嬬殑 NotoSansSC-Regular.ttf
```

## 榛樿瀛椾綋

鑻?Widget 鏈寚瀹氬瓧浣擄紝鍒欎娇鐢ㄧ涓€涓鍙戠幇鐨勫瓧浣撲綔涓洪粯璁ゅ瓧浣撱€?

## 瀛椾綋鎼滅储璺緞

`FontManager` 鎼滅储閰嶇疆鐨勭洰褰曪紙榛樿锛歚fonts/`锛夛紝浠庡伐浣滅洰褰曞紑濮嬶紝鐒跺悗鍚戜笂鍥為€€锛?

- `<font_dir>/`
- `../<font_dir>/`
- `../../<font_dir>/`

杩欑‘淇濇棤璁哄簲鐢ㄤ粠椤圭洰鏍圭洰褰曡繕鏄瀯寤鸿緭鍑虹洰褰曡繍琛岋紝閮借兘鎵惧埌瀛椾綋銆傞€氳繃 `Application::SetFontDir()` 鍦?`Init()` 涔嬪墠閰嶇疆鐩綍銆?

## 宸ヤ綔鍘熺悊

1. **瀛椾綋鍔犺浇**锛歚FontManager` 鎵弿閰嶇疆鐨勫瓧浣撶洰褰曪紝鐢?FreeType 鍔犺浇姣忎釜瀛椾綋銆?
2. **瀛楀舰鍏夋爡鍖?*锛氬瓧绗﹂娆℃覆鏌撴椂锛孎reeType 灏嗗叾鍏夋爡鍖栦负鐏板害浣嶅浘銆?
3. **绾圭悊鍥鹃泦**锛氫綅鍥句笂浼犲埌 1024x1024 绾圭悊鍥鹃泦銆?
4. **娓叉煋**锛氭瘡涓瓧褰㈢粯鍒朵负甯︾汗鐞嗙殑鍥涜竟褰€傜墖娈电潃鑹插櫒閲囨牱鍥鹃泦鐨勭孩鑹查€氶亾骞朵箻浠ユ枃鏈鑹层€?

## CJK 涓?Unicode

NeoFlux 鏀寔 UTF-8 鏂囨湰銆俙Text` Widget 鎺ュ彈 `std::string`锛圲TF-8锛夊苟娓叉煋姣忎釜 Unicode 鐮佺偣銆傛覆鏌撲腑鏂囬渶浣跨敤鍖呭惈 CJK 瀛楀舰鐨勫瓧浣擄紙濡?Noto Sans SC锛夈€?

```cpp
auto text = std::make_shared<Text>(u8"浣犲ソ涓栫晫");
text->SetFont("NotoSansSC-Regular");
```

## CMake锛氭瀯寤烘椂鑷姩鎷疯礉瀛椾綋

鍦ㄤ綘鑷繁鐨勫伐绋嬩腑锛屽皢瀛椾綋鏀惧湪 `fonts/` 鐩綍涓嬶紝閫氳繃 CMake 鍦ㄦ瘡娆℃瀯寤烘椂鎷疯礉鍒板彲鎵ц鏂囦欢鍚岀骇鐩綍锛?

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

鐒跺悗鍦ㄤ唬鐮佷腑閰嶇疆鐩綍锛?

```cpp
Application app;
app.SetFontDir("./fonts/");  // 瀵瑰簲鎷疯礉鍒拌緭鍑虹洰褰曠殑 fonts/ 鏂囦欢澶?
app.Init(argc, argv, 800, 600, "My App");
```

## 鏈€浣冲疄璺?

- 浠呭寘鍚墍闇€瀛椾綋锛屽噺灏戜簩杩涘埗浣撶Н鍜屽唴瀛樺崰鐢ㄣ€?
- 灏藉彲鑳戒娇鐢ㄥ崟涓€瀛椾綋绯诲垪鐨勫绉嶅瓧閲嶃€?
- 娓叉煋涓枃鏃讹紝纭繚瀛椾綋鍖呭惈鎵€闇€瀛楀舰銆?
- 瀛椾綋鏂囦欢閫氳繃 `.gitignore` 鎺掗櫎鍦?git 涔嬪锛涢殢搴旂敤鍒嗗彂鎴栧湪鏂囨。涓鏄庣敤鎴峰簲鏀剧疆鐨勪綅缃€?

## 涓嬩竴姝?

- [Text API](../api/text)
- [瀛椾綋婕旂ず](../examples/font)
