# 蹇€熷紑濮?

鏈寚鍗椾粠闆跺紑濮嬪垱寤轰綘鐨勭涓€涓?NeoFlux 搴旂敤銆?

## 1. 椤圭洰缁撴瀯

涓洪」鐩垱寤烘柊鐩綍銆侼eoFlux 搴旀斁鍦?`thirdparty/` 涓嬶紝灏嗕緷璧栦笌婧愮爜闅旂锛?

```
my_app/
鈹溾攢鈹€ CMakeLists.txt
鈹溾攢鈹€ main.cpp
鈹斺攢鈹€ thirdparty/
    鈹斺攢鈹€ neoflux/      # NeoFlux 婧愮爜锛坓it submodule 鎴栨嫹璐濓級
```

## 2. 鑾峰彇 NeoFlux

### 鏂瑰紡 A锛欸it Submodule锛堟帹鑽愶級

```bash
git init
git submodule add https://github.com/weiwei201906/NeoFlux.git thirdparty/neoflux
```

### 鏂瑰紡 B锛欶etchContent锛堟棤闇€ submodule锛?

鍦?`CMakeLists.txt` 涓坊鍔狅紙瑙佷笅鏂囷級鈥斺€擭eoFlux 鍦ㄩ厤缃椂鑷姩涓嬭浇鍒?`thirdparty/`銆?

## 3. main.cpp

```cpp
#include <neoflux/app/application.h>
#include <neoflux/widget/container.h>
#include <neoflux/widget/text.h>
#include <neoflux/widget/button.h>
#include <neoflux/widget/route_registry.h>

using namespace neoflux;

// 璺敱鏋勫缓鍑芥暟锛氳繑鍥?"/" 璺敱鐨勬牴 widget 鏍?
std::shared_ptr<Widget> BuildHomePage(BuildContext& /*ctx*/) {
  auto root = std::make_shared<Container>();
  root->SetFlexDirection(FlexDirection::kColumn)
      .SetBackgroundColor({.r = 245, .g = 245, .b = 245, .a = 255})
      .SetJustifyContent(HAlign::kCenter)
      .SetAlignItems(VAlign::kCenter)
      .SetPadding({.left = 24, .top = 24, .right = 24, .bottom = 24});

  auto title = std::make_shared<Text>("Hello NeoFlux");
  title->SetFontSize(28.0F)
      .SetTextColor({.r = 33, .g = 33, .b = 33, .a = 255});
  root->AddChild(title);

  auto button = std::make_shared<Button>("Click Me");
  button->SetOnPressed([]() {
    LOG(INFO) << "Button pressed!";
  });
  root->AddChild(button);

  return root;
}

int main(int argc, char** argv) {
  // 鍦ㄥ垵濮嬪寲搴旂敤鍓嶆敞鍐岃矾鐢便€?
  RouteRegistry::Instance().RegisterRoute("/", BuildHomePage);

  Application app;
  // 鍦?Init() 涔嬪墠閰嶇疆瀛椾綋鐩綍銆傚皢 .ttf/.otf 鏂囦欢鏀惧叆 fonts/
  // 锛堟垨浣犺嚜瀹氫箟鐨勭洰褰曪級銆傝瑙佸瓧浣撶郴缁熸枃妗ｃ€?
  app.SetFontDir("./fonts/");
  if (!app.Init(argc, argv, 480, 360, "My First NeoFlux App")) {
    return 1;
  }

  // 鍘嬪叆鍒濆璺敱骞惰繍琛屼簨浠跺惊鐜紙闃诲鐩村埌绐楀彛鍏抽棴锛夈€?
  app.PushRoute("/");
  app.Run();
  return 0;
}
```

:::tip
鍝€曞彧鏈変竴涓矾鐢憋紝涔熷繀椤诲厛 `RegisterRoute` 鍐?`PushRoute`銆俙Init` 鍙垱寤虹獥鍙ｂ€斺€斿湪鎺ㄩ€佽矾鐢变箣鍓嶄笉浼氭樉绀轰换浣?Widget銆?
:::

:::warning
鏂囨湰 Widget 闇€瑕佸瓧浣撴枃浠躲€傝繍琛屽墠璇峰湪閰嶇疆鐨勫瓧浣撶洰褰曪紙榛樿 `fonts/`锛変腑鑷冲皯鏀惧叆涓€涓?`.ttf`/`.otf` 瀛椾綋銆傛病鏈夊瓧浣撴椂锛屾墍鏈夋枃鏈細鏄剧ず涓轰贡鐮佹垨绌虹櫧銆?
:::

## 4. CMakeLists.txt

### 浣跨敤 submodule

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NeoFlux 鏀惧湪 thirdparty/ 涓嬶紝闅旂渚濊禆銆?
add_subdirectory(thirdparty/neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)

# 姣忔鏋勫缓鏃跺皢 fonts/ 鐩綍鎷疯礉鍒板彲鎵ц鏂囦欢鍚岀骇鐩綍銆?
# 灏嗕綘鐨?.ttf/.otf 鏂囦欢鏀惧叆 ${CMAKE_SOURCE_DIR}/fonts/
add_custom_command(TARGET my_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:my_app>/fonts
)
```

> **娉ㄦ剰锛?* NeoFlux 鐨勭ず渚嬪拰娴嬭瘯**榛樿鍏抽棴**銆傚闇€鏋勫缓锛屽湪閰嶇疆鏃朵紶鍏?
> `-DNEOFLUX_BUILD_EXAMPLES=ON -DNEOFLUX_BUILD_TESTS=ON`銆?
>
> **瀛椾綋锛?* 杩愯绀轰緥闇€瑕?`thirdparty/fonts/` 鐩綍涓嬫湁瀛椾綋鏂囦欢锛堢ず渚嬫樉寮忚皟鐢?SetFontDir("./fonts/")锛夈€傛病鏈夊瓧浣撴椂锛屾枃鏈細鏄剧ず涓轰贡鐮佹垨绌虹櫧銆?

### 浣跨敤 FetchContent

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  neoflux
  GIT_REPOSITORY https://github.com/weiwei201906/NeoFlux.git
  GIT_TAG main
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/neoflux
)
FetchContent_MakeAvailable(neoflux)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE neoflux)
```

## 5. 鏋勫缓骞惰繍琛?

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
./my_app
```

浣犲簲璇ョ湅鍒颁竴涓獥鍙ｏ紝鏄剧ず "Hello NeoFlux" 鏂囨湰鍜屼竴涓彲鐐瑰嚮鐨勬寜閽€?

## 鏍稿績姒傚康

| 姒傚康 | 璇存槑 |
|------|------|
| **璺敱鏋勫缓鍑芥暟** | 杩斿洖 widget 鏍戠殑鍑芥暟锛岃矾鐢辫鍘嬪叆鏃惰皟鐢ㄣ€?|
| **Container** | 鍩虹甯冨眬缁勪欢銆侳lex 鏂瑰悜銆佸榻愩€佸唴杈硅窛銆佽儗鏅壊銆?|
| **Text** | 娓叉煋 UTF-8 鏂囨湰鐨勫彾瀛愮粍浠躲€?|
| **Button** | 鍙偣鍑荤粍浠讹紝閫氳繃 `SetOnPressed()` 璁剧疆鍥炶皟銆?|
| **RouteRegistry** | 灏嗚矾鐢卞悕绉版槧灏勫埌鏋勫缓鍑芥暟銆?|
| **Application** | 鎷ユ湁绐楀彛銆佷簨浠跺惊鐜€佹覆鏌撳眰鍜屽鑸爤銆?|

## 涓嬩竴姝?

- 瀛︿範 [Widget 绯荤粺](./widgets)
- 鎺㈢储 [Flex 甯冨眬](./layout)
- 澶勭悊 [鐢ㄦ埛杈撳叆](./input)
- 娣诲姞椤甸潰闂?[璺敱瀵艰埅](./routing)
- 浣跨敤 [鍗忕▼](./coroutines) 瀹炵幇鍔ㄧ敾鍜屽畾鏃跺伐浣?
