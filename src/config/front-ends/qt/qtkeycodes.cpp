#include <rsys/keyboard.h>
#include <unordered_map>
#include <QtCore>

namespace Executor
{
std::unordered_map<Qt::Key,int> qtToMacKeycodeMap {
    { Qt::Key_Backspace, MKV_BACKSPACE },
    { Qt::Key_Tab, MKV_TAB },
    //{ Qt::Key_Clear, NOTAKEY },
    { Qt::Key_Return, MKV_RETURN },
    { Qt::Key_Escape, MKV_ESCAPE },
    { Qt::Key_Space, MKV_SPACE },
    { Qt::Key_Apostrophe, MKV_TICK },
    { Qt::Key_Comma, MKV_COMMA },
    { Qt::Key_Minus, MKV_MINUS },
    { Qt::Key_Period, MKV_PERIOD },
    { Qt::Key_Slash, MKV_SLASH },
    { Qt::Key_0, MKV_0 },
    { Qt::Key_1, MKV_1 },
    { Qt::Key_2, MKV_2 },
    { Qt::Key_3, MKV_3 },
    { Qt::Key_4, MKV_4 },
    { Qt::Key_5, MKV_5 },
    { Qt::Key_6, MKV_6 },
    { Qt::Key_7, MKV_7 },
    { Qt::Key_8, MKV_8 },
    { Qt::Key_9, MKV_9 },
    { Qt::Key_Semicolon, MKV_SEMI },
    { Qt::Key_Colon, MKV_SEMI },
    { Qt::Key_Equal, MKV_EQUAL },
    /*{ Qt::Key_Kp_0, MKV_NUM0 },
    { Qt::Key_Kp_1, MKV_NUM1 },
    { Qt::Key_Kp_2, MKV_NUM2 },
    { Qt::Key_Kp_3, MKV_NUM3 },
    { Qt::Key_Kp_4, MKV_NUM4 },
    { Qt::Key_Kp_5, MKV_NUM5 },
    { Qt::Key_Kp_6, MKV_NUM6 },
    { Qt::Key_Kp_7, MKV_NUM7 },
    { Qt::Key_Kp_8, MKV_NUM8 },
    { Qt::Key_Kp_9, MKV_NUM9 },
    { Qt::Key_Kp_PERIOD, MKV_NUMPOINT },
    { Qt::Key_Kp_DIVIDE, MKV_NUMDIVIDE },
    { Qt::Key_Kp_MULTIPLY, MKV_NUMMULTIPLY },
    { Qt::Key_Kp_MINUS, MKV_NUMMINUS },
    { Qt::Key_Kp_PLUS, MKV_NUMPLUS },*/
    { Qt::Key_Enter, MKV_NUMENTER },
    { Qt::Key_BracketLeft, MKV_LEFTBRACKET },
    { Qt::Key_Backslash, MKV_BACKSLASH },
    { Qt::Key_BracketRight, MKV_RIGHTBRACKET },
    { Qt::Key_QuoteLeft, MKV_BACKTICK },
    { Qt::Key_BraceLeft, MKV_LEFTBRACKET },
    { Qt::Key_BraceRight, MKV_RIGHTBRACKET },
    { Qt::Key_section, MKV_PARAGRAPH },
    { Qt::Key_A, MKV_a },
    { Qt::Key_B, MKV_b },
    { Qt::Key_C, MKV_c },
    { Qt::Key_D, MKV_d },
    { Qt::Key_E, MKV_e },
    { Qt::Key_F, MKV_f },
    { Qt::Key_G, MKV_g },
    { Qt::Key_H, MKV_h },
    { Qt::Key_I, MKV_i },
    { Qt::Key_J, MKV_j },
    { Qt::Key_K, MKV_k },
    { Qt::Key_L, MKV_l },
    { Qt::Key_M, MKV_m },
    { Qt::Key_N, MKV_n },
    { Qt::Key_O, MKV_o },
    { Qt::Key_P, MKV_p },
    { Qt::Key_Q, MKV_q },
    { Qt::Key_R, MKV_r },
    { Qt::Key_S, MKV_s },
    { Qt::Key_T, MKV_t },
    { Qt::Key_U, MKV_u },
    { Qt::Key_V, MKV_v },
    { Qt::Key_W, MKV_w },
    { Qt::Key_X, MKV_x },
    { Qt::Key_Y, MKV_y },
    { Qt::Key_Z, MKV_z },
    { Qt::Key_Delete, MKV_DELFORWARD },
    { Qt::Key_F1, MKV_F1 },
    { Qt::Key_F2, MKV_F2 },
    { Qt::Key_F3, MKV_F3 },
    { Qt::Key_F4, MKV_F4 },
    { Qt::Key_F5, MKV_F5 },
    { Qt::Key_F6, MKV_F6 },
    { Qt::Key_F7, MKV_F7 },
    { Qt::Key_F8, MKV_F8 },
    { Qt::Key_F9, MKV_F9 },
    { Qt::Key_F10, MKV_F10 },
    { Qt::Key_F11, MKV_F11 },
    { Qt::Key_F12, MKV_F12 },
    { Qt::Key_F13, MKV_F13 },
    { Qt::Key_F14, MKV_F14 },
    { Qt::Key_F15, MKV_F15 },
    { Qt::Key_Pause, MKV_PAUSE },
    { Qt::Key_NumLock, MKV_NUMCLEAR },
    { Qt::Key_Up, MKV_UPARROW },
    { Qt::Key_Down, MKV_DOWNARROW },
    { Qt::Key_Right, MKV_RIGHTARROW },
    { Qt::Key_Left, MKV_LEFTARROW },
    { Qt::Key_Insert, MKV_HELP },
    { Qt::Key_Home, MKV_HOME },
    { Qt::Key_End, MKV_END },
    { Qt::Key_PageUp, MKV_PAGEUP },
    { Qt::Key_PageDown, MKV_PAGEDOWN },
    { Qt::Key_CapsLock, MKV_CAPS },
    { Qt::Key_ScrollLock, MKV_SCROLL_LOCK },
    { Qt::Key_Shift, MKV_LEFTSHIFT },
#ifdef MACOSX
    { Qt::Key_Control, MKV_CLOVER },
    { Qt::Key_Alt, MKV_LEFTOPTION },
    { Qt::Key_Meta, MKV_LEFTCNTL },
#else
    { Qt::Key_Control, MKV_CLOVER },
    { Qt::Key_Alt, MKV_LEFTOPTION },
    { Qt::Key_Meta, MKV_LEFTCNTL },
#endif
    //{ Qt::Key_Help, MKV_HELP },
    { Qt::Key_Print, MKV_PRINT_SCREEN },
};
}