#include "gui_generated/containers/menuItemContainerBase.hpp"
#include "images/BitmapDatabase.hpp"
#include <cstdint>
#include <gui/containers/menuItemContainer.hpp>
#include <texts/TextKeysAndLanguages.hpp>   /* T_XXX 文字ID在此声明 */

menuItemContainer::menuItemContainer() {}

void menuItemContainer::initialize() { menuItemContainerBase::initialize(); }

void menuItemContainer::updateItem(int16_t itemIndex) {
  switch (itemIndex) {
  case 0:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINLEFT_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_LIGHTNING));//手电
    break;
  case 1:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINMIDDLE_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_ALARM));//闹钟
    break;
  case 2:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINRIGHT_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_HEARTRATE));//心率
    break;
  case 3:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINWINK_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_SETTING)); // 设置
    break;
  case 4:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINSTAREYED_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_CHRONOGRAPH));//秒表
    break;
  case 5:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_INDEX_ICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_INDEX));//谨遵指令之意
    break;
  case 6:
    Icon.setBitmap(touchgfx::Bitmap(BITMAP_MIUYINTIMID_FORICON_ID));
    Title.setTypedText(touchgfx::TypedText(T_DINOSUAR));//缪因快跑// 假如你不设置这个图像的话，滑动菜单控件
    break;
  default:
    break;
  }
  Icon.invalidate(); // 刷新图片
  Title.invalidate(); // 刷新文字
}