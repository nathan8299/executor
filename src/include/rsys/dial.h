#if !defined(__rsys_dial_h__)
#define __rsys_dial_h__
namespace Executor
{
struct icon_item_template_t
{
    GUEST_STRUCT;
    GUEST<int16_t> count;
    GUEST<Handle> h;
    GUEST<Rect> r;
    GUEST<uint8> type;
    GUEST<uint8> len;
    GUEST<int16_t> res_id;
};
}
#endif /* !defined (__rsys_dial_h__) */
