/**
 ******************************************************************************
 * @file    SC7A20_reg.h
 * @brief   SC7A20HTR 鍔犻€熷害璁″瘎瀛樺櫒瀹氫箟澶存枃浠?
 ******************************************************************************
 * @note    鏈枃浠跺唴瀹氫箟浜?SC7A20HTR 鍔犻€熷害璁＄殑瀵勫瓨鍣ㄥ湴鍧€銆?
 *          瀵勫瓨鍣ㄤ綅鍩熺粨鏋勪綋銆佸瘎瀛樺櫒浣嶅煙缁撴瀯浣撳畾涔夌瓑銆?
 ******************************************************************************
 */

#ifndef SC7A20_REG_H
#define SC7A20_REG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ========================== 璁惧鍩烘湰淇℃伅 ========================== */
#define SC7A20_CHIP_ID     0x11 // 璁惧ID瀵勫瓨鍣ㄩ鏈熷€?
#define SC7A20_VERSION_VAL 0x28 // 鐗堟湰鍙峰瘎瀛樺櫒棰勬湡鍊?
// I2C鍦板潃
#define SC7A20_I2C_ADDR_L 0x18 //  SDO鎺ラ€昏緫浣?
#define SC7A20_I2C_ADDR_H 0x19 //  SDO鎮┖/鎺ラ€昏緫楂?

/* ========================== 鏍稿績瀵勫瓨鍣ㄥ湴鍧€ ========================== */
/* 璁惧璇嗗埆 */
#define SC7A20_WHO_AM_I 0x0F // 璁惧ID瀵勫瓨鍣?
#define SC7A20_VERSION  0x70 // 鐗堟湰鍙峰瘎瀛樺櫒
/* 鎺у埗瀵勫瓨鍣ㄧ粍 */
#define SC7A20_CTRL0      0x1F // 鎺у埗瀵勫瓨鍣?锛堟ā寮忔帶鍒讹級
#define SC7A20_CTRL1      0x20 // 鎺у埗瀵勫瓨鍣?锛堝姞閫熷害璁￠厤缃級
#define SC7A20_CTRL2      0x21 // 鎺у埗瀵勫瓨鍣?锛堟护娉㈠櫒閰嶇疆锛?
#define SC7A20_CTRL3      0x22 // 鎺у埗瀵勫瓨鍣?锛堜腑鏂厤缃級
#define SC7A20_CTRL4      0x23 // 鎺у埗瀵勫瓨鍣?锛堟暟鎹缃級
#define SC7A20_CTRL5      0x24 // 鎺у埗瀵勫瓨鍣?
#define SC7A20_CTRL6      0x25 // 鎺у埗瀵勫瓨鍣?

#define SC7A20_SOFT_RESET 0x68 // 杞浣?(瀵勫瓨鍣ㄥ啓鍏?xA5锛屽浣嶆暣涓數璺紝鏁版嵁娓呴浂)
#define SC7A20_I2C_CTRL   0x6F // I2C鎺у埗瀵勫瓨鍣?

#define SC7A20_SPI_CTRL   0x0E // SPI鎺у埗瀵勫瓨鍣?
/* 鐘舵€佸瘎瀛樺櫒 */
#define SC7A20_DRDY_STATUS 0x27 // 鐘舵€佸瘎瀛樺櫒
/* ========================== 鏁版嵁杈撳嚭瀵勫瓨鍣?========================== */
/* 鍔犻€熷害璁¤緭鍑?*/
#define SC7A20_OUTX_L 0x28 // 鍔犻€熷害璁杞翠綆瀛楄妭
#define SC7A20_OUTX_H 0x29 // 鍔犻€熷害璁杞撮珮瀛楄妭
#define SC7A20_OUTY_L 0x2A // 鍔犻€熷害璁杞翠綆瀛楄妭
#define SC7A20_OUTY_H 0x2B // 鍔犻€熷害璁杞撮珮瀛楄妭
#define SC7A20_OUTZ_L 0x2C // 鍔犻€熷害璁杞翠綆瀛楄妭
#define SC7A20_OUTZ_H 0x2D // 鍔犻€熷害璁杞撮珮瀛楄妭

/* 瀹炴椂鐨刋杞村姞閫熷害璁″€?*/
#define SC7A20_OUTX_New_L 0x61 // 瀹炴椂X杞村姞閫熷害璁′綆瀛楄妭
#define SC7A20_OUTX_New_H 0x62 // 瀹炴椂X杞村姞閫熷害璁￠珮瀛楄妭
/* 瀹炴椂Y杞村姞閫熷害璁″€?*/
#define SC7A20_OUTY_New_L 0x63 // 瀹炴椂Y杞村姞閫熷害璁′綆瀛楄妭
#define SC7A20_OUTY_New_H 0x64 // 瀹炴椂Y杞村姞閫熷害璁￠珮瀛楄妭
/* 瀹炴椂Z杞村姞閫熷害璁″€?*/
#define SC7A20_OUTZ_New_L 0x65 // 瀹炴椂Z杞村姞閫熷害璁′綆瀛楄妭
#define SC7A20_OUTZ_New_H 0x66 // 瀹炴椂Z杞村姞閫熷害璁￠珮瀛楄妭
/* ========================== FIFO鎺у埗瀵勫瓨鍣?========================== */
#define SC7A20_FIFO_CTRL 0x2E // FIFO鎺у埗瀵勫瓨鍣?
#define SC7A20_FIFO_SRC  0x2F // FIFO鐘舵€佸瘎瀛樺櫒
#define SC7A20_FIFO_DATA 0x69 // FIFO鏁版嵁瀵勫瓨鍣?
/*
 * 璇诲彇 SC7A20_FIFO_DATA 瀵勫瓨鍣ㄧ浉褰撲簬鏄鍙朏IFO鏁版嵁锛岃鍙栨暟鎹『搴忔槸X杞淬€乊杞淬€乑杞达紱
 * 鍙互鏍规嵁0x2F瀵勫瓨鍣ㄥ€艰绠桭IFO缁勬暟锛岀劧鍚庣粍鏁?3浣滀负璇诲彇0x69鐨勬鏁帮紱
 * FIFO_MODE=0鐩稿綋浜庨『搴忔槸28h,29h,2Ah,2Bh,2Ch,2Dh锛?
 * FIFO_MODE=1鐩稿綋浜庨『搴忔槸28h,2Ah,2Ch銆?
 */

/* ========================== 涓柇鎺у埗瀵勫瓨鍣?========================== */
#define SC7A20_INT1_CFG 0x30 // INT1閰嶇疆瀵勫瓨鍣?
#define SC7A20_INT1_SRC 0x31 // INT1鐘舵€佸瘎瀛樺櫒
#define SC7A20_INT1_THS 0x32 // INT1闃堝€煎瘎瀛樺櫒
#define SC7A20_INT1_DUR 0x33 // INT1鎸佺画鏃堕棿瀵勫瓨鍣?

#define SC7A20_INT2_CFG 0x34 // INT2閰嶇疆瀵勫瓨鍣?
#define SC7A20_INT2_SRC 0x35 // INT2鐘舵€佸瘎瀛樺櫒
#define SC7A20_INT2_THS 0x36 // INT2闃堝€煎瘎瀛樺櫒
#define SC7A20_INT2_DUR 0x37 // INT2鎸佺画鏃堕棿瀵勫瓨鍣?
/* ========================== 杩愬姩妫€娴嬪瘎瀛樺櫒 ========================== */
#define SC7A20_CLICK_CRTL   0x38 // 鏁插嚮鎺у埗瀵勫瓨鍣?
#define SC7A20_CLICK_SRC    0x39 // 鏁插嚮鐘舵€佸瘎瀛樺櫒
#define SC7A20_CLICK_COEFF1 0x3A // 鏁插嚮绯绘暟瀵勫瓨鍣?
#define SC7A20_CLICK_COEFF2 0x3B // 鏁插嚮绯绘暟瀵勫瓨鍣?
#define SC7A20_CLICK_COEFF3 0x3C // 鏁插嚮绯绘暟瀵勫瓨鍣?
#define SC7A20_CLICK_COEFF4 0x3D // 鏁插嚮绯绘暟瀵勫瓨鍣?

#define SC7A20_DIG_CTRL     0x57 // 鏁板瓧鍔熻兘鎺у埗瀵勫瓨鍣?

/* ========================== 瀵勫瓨鍣ㄤ綅鍩熺粨鏋勪綋 ========================== */

/* CTRL0 (0x1F): 妯″紡鎺у埗 */
typedef union {
    struct {
        uint8_t HR : 1;   // 宸ヤ綔妯″紡鎺у埗浣?(0:浣庡姛鑰? 1:楂樺垎杈ㄧ巼  -->璇峰弬鑰?0h瀵勫瓨鍣ㄨ鏄?
        uint8_t DLPF : 1; // 鏁板瓧浣庨€氭护娉㈠櫒鎺у埗浣嶉珮浣嶃€?
        uint8_t : 1;      // 淇濈暀浣?
        uint8_t : 1;      // 淇濈暀浣?
        uint8_t OSR : 3;  // 鏁版嵁鏇存柊閫熺巼鎺у埗浣?(000: ODR, 001: ODR/2, 010: ODR/4, 011:ODR/8, 100:ODR/16, 101~111: ODR/32)
        uint8_t : 1;      // 淇濈暀浣?
    } bit;
    uint8_t reg;
} sc7a20_ctrl0_t;

/* CTRL1 (0x20): 鍔犻€熷害璁￠厤缃?*/
/*
 * 鏁版嵁杈撳嚭鐜囩殑閰嶇疆
 * 0000: 鐢垫簮鍏虫柇妯″紡           0001: 鍏ㄥ伐浣滄ā寮?1.56Hz)
 * 0010: 鍏ㄥ伐浣滄ā寮?12.5Hz)     0011: 鍏ㄥ伐浣滄ā寮?25Hz)
 * 0100: 鍏ㄥ伐浣滄ā寮?50Hz)       0101: 鍏ㄥ伐浣滄ā寮?100Hz)
 * 0110: 鍏ㄥ伐浣滄ā寮?200Hz)      0111: 鍏ㄥ伐浣滄ā寮?400Hz)
 * 1000: 鍏ㄥ伐浣滄ā寮?800Hz)      1001: 楂樻€ц兘妯″紡(1.48kHz)
 * 1010: 楂樻€ц兘妯″紡(2.66kHz)    1011: 楂樻€ц兘妯″紡(4.434kHz)
 *
 * 宸ヤ綔妯″紡閰嶇疆
 * HR=0, LPen=0: 姝ｅ父妯″紡
 * HR=0, LPen=1: 浣庡姛鑰楁ā寮?
 * HR=1, LPen=0: 楂樻€ц兘妯″紡
 * HR=1, LPen=1: 澧炲己妯″紡
 *
 */

typedef union {
    struct {
        uint8_t Xen : 1;  // X杞翠娇鑳戒綅
        uint8_t Yen : 1;  // Y杞翠娇鑳戒綅
        uint8_t Zen : 1;  // Z杞翠娇鑳戒綅
        uint8_t LPen : 1; // 浣庡姛鑰楁ā寮忎娇鑳戒綅
        uint8_t ODR : 4;  // 鏁版嵁鐜囬€夋嫨
    } bit;
    uint8_t reg;
} sc7a20_ctrl1_t;

/* CTRL2 (0x21): 婊ゆ尝鍣ㄩ厤缃?*/
/*
 * 鏁版嵁杈撳嚭鐜囩殑閰嶇疆
 * HPCF | Ft[Hz]@12.5Hz | Ft[Hz]@25Hz | Ft[Hz]@50Hz | Ft[Hz]@100Hz | Ft[Hz]@200Hz | Ft[Hz]@400Hz
 *  00      0.8              2               4               8           16              32
 *  01      0.32            0.8              2               4            8              16
 *  10      0.04            0.1             0.2             0.5           1               2
 *  11      0.02            0.05            0.1             0.2          0.5              1
 */
typedef union {
    struct {
        uint8_t HPIS1 : 1;    // 涓柇AOI1鍔熻兘楂橀€氭护娉娇鑳?
        uint8_t HPIS2 : 1;    // 涓柇AOI2鍔熻兘楂橀€氭护娉娇鑳?
        uint8_t HP_reset : 1; // 楂橀€氭护娉㈠櫒澶嶄綅
        uint8_t FDS : 1;      // 鏁版嵁婊ゆ尝閫夋嫨
        uint8_t HPCF : 2;     // 楂橀€氭埅姝㈤鐜囬€夋嫨
        uint8_t HDS : 1;      // 楂橀€氭护娉㈠櫒鏁版嵁閫夋嫨
        uint8_t : 1;          // 淇濈暀浣?
    } bit;
    uint8_t reg;
} sc7a20_ctrl2_t;

/* CTRL3 (0x22): 涓柇閰嶇疆 */
typedef union {
    struct {
        uint8_t fifo_mode : 1;    // FIFO data width mode
        uint8_t int1_overrun : 1; // FIFO overrun interrupt on INT1
        uint8_t int1_wtm : 1;     // FIFO watermark interrupt on INT1
        uint8_t : 1;              // reserved
        uint8_t int1_drdy : 1;    // DRDY interrupt on INT1
        uint8_t int1_aoi2 : 1;    // AOI2 interrupt on INT1
        uint8_t int1_aoi1 : 1;    // AOI1 interrupt on INT1
        uint8_t int1_click : 1;   // CLICK interrupt on INT1
    } bit;
    uint8_t reg;
} sc7a20_ctrl3_t;

/* CTRL4 (0x23):  鏁版嵁璁剧疆 */
typedef union {
    struct {
        uint8_t sim : 1;  // SPI 涓茶鎺ュ彛妯″紡閰嶇疆
        uint8_t st : 2;   // 鑷祴璇曚娇鑳?
        uint8_t DLPF : 1; // 鏁板瓧浣庨€氭护娉㈠櫒鎺у埗浣嶄綆浣?
        uint8_t fs : 2;   // 鍏ㄩ噺绋嬮€夋嫨
        uint8_t BLE : 1;  // 鏁版嵁瀛楄妭搴忛€夋嫨 (0锛氫綆瀛楄妭鏁版嵁鍦ㄤ綆鍦板潃锛?锛氶珮瀛楄妭鏁版嵁鍦ㄤ綆鍦板潃)
        uint8_t BDU : 1;  // 鍧楁暟鎹洿鏂?(0锛氳繛缁洿鏂帮紱1锛氳緭鍑烘暟鎹瘎瀛樺櫒涓嶆洿鏂扮洿鍒癕SB鍜孡SB琚鍙?
    } bit;
    uint8_t reg;
} sc7a20_ctrl4_t;

/* CTRL5 (0x24): 閰嶇疆瀵勫瓨鍣?5 */
typedef union {
    struct {
        uint8_t D4D_INT2 : 1; // 4D浣胯兘锛氬湪INT2绠¤剼涓婁娇鑳?D妫€娴嬶紝鍚屾椂瑕佹妸涓柇2閰嶇疆瀵勫瓨鍣ㄤ腑鐨?D涓虹疆1銆?
        uint8_t LIR_INT2 : 1; // 閿佸瓨涓柇2閰嶇疆瀵勫瓨鍣ㄤ笂鎸囧畾鐨勪腑鏂搷搴?(0锛氫笉閿佸瓨涓柇淇″彿锛?锛氶攣瀛樹腑鏂俊鍙?
        uint8_t D4D_INT1 : 1; // 4D浣胯兘锛氬湪INT1绠¤剼涓婁娇鑳?D妫€娴嬶紝鍚屾椂瑕佹妸涓柇1閰嶇疆瀵勫瓨鍣ㄤ腑鐨?D涓虹疆1銆?
        uint8_t LIR_INT1 : 1; // Latched interrupt 1 response
        uint8_t : 1;          // reserved
        uint8_t AOI_EN : 1;   // AOI interrupt disable
        uint8_t FIFO_EN : 1;  // FIFO enable
        uint8_t boot : 1;     // 閲嶈浇淇皟鍊?
    } bit;
    uint8_t reg;
} sc7a20_ctrl5_t;

/* CTRL6 (0x25): 涓柇鎺у埗 */
typedef union {
    struct {
        uint8_t INT_PP_OD : 1; // INT1鍜孖NT2鎺ㄦ尳杈撳嚭鎴栧紑婕忚緭鍑洪€夋嫨浣?
        uint8_t H_LACTIVE : 1; // 涓柇寮曡剼榛樿鐢靛钩鎺у埗浣?
        uint8_t CS_PU_EN : 1;  // CS寮曡剼涓婃媺鐢甸樆浣胯兘浣?
        uint8_t I2_DRDY : 1;   // DRDY涓柇鍦↖NT2涓?
        uint8_t I2_BOOT : 1;   // BOOT鐘舵€佸湪INT2涓?
        uint8_t I2_AOI2 : 1;   // AOI2涓柇鍦↖NT2涓?
        uint8_t I2_AOI1 : 1;   // AOI1涓柇鍦↖NT2涓?
        uint8_t I2_CLICK : 1;  // CLICK涓柇鍦↖NT2涓?
    } bit;
    uint8_t reg;
} sc7a20_ctrl6_t;

/* DRDY_STATUS (0x27): 鐘舵€佸瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t XDA : 1;   // X杞存暟鎹彲鐢?
        uint8_t YDA : 1;   // Y杞存暟鎹彲鐢?
        uint8_t ZDA : 1;   // Z杞存暟鎹彲鐢?
        uint8_t ZYXDA : 1; // X锛孻鍜孼涓変釜杞存柊鐨勬暟鎹叏閮借浆鎹㈠畬鎴?
        uint8_t XOR : 1;   // X杞存柊鐨勬暟鎹凡缁忚鐩栬€佺殑鏁版嵁
        uint8_t YOR : 1;   // Y杞存柊鐨勬暟鎹凡缁忚鐩栬€佺殑鏁版嵁
        uint8_t ZOR : 1;   // Z杞存柊鐨勬暟鎹凡缁忚鐩栬€佺殑鏁版嵁
        uint8_t ZYXOR : 1; // X锛孻鍜孼涓変釜杞存柊鐨勬暟鎹嚦灏戞湁涓€涓凡缁忚鐩栬€佺殑鏁版嵁銆?
    } bit;
    uint8_t reg;
} sc7a20_drdy_status_t;

/*  FIFO_CTRL (0x2E): FIFO鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t FTH : 5; // FIFO鍔熻兘WTM闃堝€艰缃?
        uint8_t TR : 1;  // FIFO瑙﹀彂妯″紡閫夋嫨(0:AOI1涓柇浣滀负FIFO瑙﹀彂妯″紡涓柇浜嬩欢杈撳叆, 1:AOI2涓柇浣滀负FIFO瑙﹀彂妯″紡涓柇浜嬩欢杈撳叆 )
        uint8_t FM : 2;  // FIFO妯″紡閫夋嫨 (00:鏃佽矾妯″紡, 01:FIFO妯″紡, 10:娴佹ā寮? 11:瑙﹀彂妯″紡)
    } bit;
    uint8_t reg;
} sc7a20_fifo_ctrl_t;

/*  FIFO_SRC (0x2F):  FIFO鐘舵€佸瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t FSS : 5;   // 鍦‵IFO涓湭璇诲彇鏁版嵁鐨勭粍鏁?
        uint8_t EMPTY : 1; // 褰揊IFO涓殑鏁版嵁鍏ㄩ儴琚鍙栨垨鑰匜IFO鏁版嵁涓暟涓?鏃讹紝EMPTY浣嶇疆鈥?鈥?
        uint8_t OVER : 1;  // FIFO婧㈠嚭鏍囧織 (1:鍙戠敓婧㈠嚭)
        uint8_t WTM : 1;   // 褰揊IFO涓殑鏁版嵁涓暟瓒呰繃璁惧畾闃堝€兼椂锛學TM浣嶇疆鈥?鈥?
    } bit;
    uint8_t reg;
} sc7a20_fifo_src_t;

/*  INT1_CFG (0x30):  涓柇1閰嶇疆瀵勫瓨鍣?*/
/*
 * AOI  | 6D  | 涓柇妯″紡
 *  0      0    鎴栦腑鏂簨浠?
 *  0      1    6涓柟鍚戣繍鍔ㄨ瘑鍒?
 *  1      0    涓庝腑鏂簨浠?
 *  1      1    6涓柟鍚戜綅缃娴?
 */

typedef union {
    struct {
        uint8_t XLIE_XDOWNE : 1; // X杞翠綆浜嬩欢涓柇鎴栬€匵杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t XHIE_XUPE : 1;   // X杞撮珮浜嬩欢涓柇鎴栬€匵杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t YLIE_YDOWNE : 1; // Y杞翠綆浜嬩欢涓柇鎴栬€匶杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t YHIE_XUPE : 1;   // Y杞撮珮浜嬩欢涓柇鎴栬€匶杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t ZLIE_ZDOWNE : 1; // Z杞翠綆浜嬩欢涓柇鎴栬€匷杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t ZHIE_ZUPE : 1;   // Z杞撮珮浜嬩欢涓柇鎴栬€匷杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t _6D : 1;         // 6D鏂瑰悜妫€娴嬩娇
        uint8_t AOI : 1;         // 涓柇閫昏緫妯″紡閫夋嫨 (0:OR閫昏緫, 1:AND閫昏緫)
    } bit;
    uint8_t reg;
} sc7a20_int1_cfg_t;

/*  INT1_SRC (0x31):  涓柇1鐘舵€佸瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t XL : 1; // X杞翠綆 (0锛氭病鏈変腑鏂紝1锛歑杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t XH : 1; // X杞撮珮 (0锛氭病鏈変腑鏂紝1锛歑杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t YL : 1; // Y杞翠綆 (0锛氭病鏈変腑鏂紝1锛歒杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t YH : 1; // Y杞撮珮 (0锛氭病鏈変腑鏂紝1锛歒杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t ZL : 1; // Z杞翠綆 (0锛氭病鏈変腑鏂紝1锛歓杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t ZH : 1; // Z杞撮珮 (0锛氭病鏈変腑鏂紝1锛歓杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t IA : 1; // 涓柇婵€娲?(0锛氭病鏈変腑鏂紝1锛氫腑鏂凡缁忎骇鐢?
        uint8_t : 1;    // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int1_src_t;

/*  INT2_THS (0x32):  涓柇1闃堝€煎瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t THS : 7; // 涓柇1闃堝€?
        uint8_t : 1;     // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int1_ths_t;

/*  INT1_DUR (0x33):  涓柇1鎸佺画鏃堕棿 */
typedef union {
    struct {
        uint8_t D : 7; // 鎸佺画鏃堕棿璁℃暟鍊?
        uint8_t : 1;   // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int1_dur_t;

/*  INT2_CFG (0x34):  涓柇2閰嶇疆瀵勫瓨鍣?*/
/*
 * AOI  | 6D  | 涓柇妯″紡
 *  0      0    鎴栦腑鏂簨浠?
 *  0      1    6涓柟鍚戣繍鍔ㄨ瘑鍒?
 *  1      0    涓庝腑鏂簨浠?
 *  1      1    6涓柟鍚戜綅缃娴?
 */

typedef union {
    struct {
        uint8_t XLIE_XDOWNE : 1; // X杞翠綆浜嬩欢涓柇鎴栬€匵杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t XHIE_XUPE : 1;   // X杞撮珮浜嬩欢涓柇鎴栬€匵杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t YLIE_YDOWNE : 1; // Y杞翠綆浜嬩欢涓柇鎴栬€匶杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t YHIE_XUPE : 1;   // Y杞撮珮浜嬩欢涓柇鎴栬€匶杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t ZLIE_ZDOWNE : 1; // Z杞翠綆浜嬩欢涓柇鎴栬€匷杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t ZHIE_ZUPE : 1;   // Z杞撮珮浜嬩欢涓柇鎴栬€匷杞存柟鍚戞娴嬩腑鏂娇鑳?
        uint8_t _6D : 1;         // 6D鏂瑰悜妫€娴嬩娇
        uint8_t AOI : 1;         // 涓柇閫昏緫妯″紡閫夋嫨 (0:OR閫昏緫, 1:AND閫昏緫)
    } bit;
    uint8_t reg;
} sc7a20_int2_cfg_t;

/*  INT2_SRC (0x35):  涓柇2鐘舵€佸瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t XL : 1; // X杞翠綆 (0锛氭病鏈変腑鏂紝1锛歑杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t XH : 1; // X杞撮珮 (0锛氭病鏈変腑鏂紝1锛歑杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t YL : 1; // Y杞翠綆 (0锛氭病鏈変腑鏂紝1锛歒杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t YH : 1; // Y杞撮珮 (0锛氭病鏈変腑鏂紝1锛歒杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t ZL : 1; // Z杞翠綆 (0锛氭病鏈変腑鏂紝1锛歓杞翠綆浜嬩欢宸茬粡浜х敓)
        uint8_t ZH : 1; // Z杞撮珮 (0锛氭病鏈変腑鏂紝1锛歓杞撮珮浜嬩欢宸茬粡浜х敓)
        uint8_t IA : 1; // 涓柇婵€娲?(0锛氭病鏈変腑鏂紝1锛氫腑鏂凡缁忎骇鐢?
        uint8_t : 1;    // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int2_src_t;

/*  INT2_THS (0x36):  涓柇2闃堝€煎瘎瀛樺櫒 */
typedef union {
    struct {
        uint8_t THS : 7; // 涓柇1闃堝€?
        uint8_t : 1;     // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int2_ths_t;

/*  INT2_DUR (0x37):  涓柇2鎸佺画鏃堕棿 */
typedef union {
    struct {
        uint8_t D : 7; // 鎸佺画鏃堕棿璁℃暟鍊?
        uint8_t : 1;   // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_int2_dur_t;

/*  CLICK_CRTL (0x38):  鏁插嚮鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t CLICK_Z_EN : 1; // Z杞存暡鍑诲姛鑳戒娇鑳戒綅
        uint8_t CLICK_Y_EN : 1; // Y杞存暡鍑诲姛鑳戒娇鑳戒綅
        uint8_t CLICK_X_EN : 1; // X杞存暡鍑诲姛鑳戒娇鑳戒綅
        uint8_t LIR_CLICK : 1;  // 鏁插嚮涓柇閿佸瓨浣胯兘浣?
        uint8_t CLICK_SEL : 1;  // 0锛氭暡鍑讳簨浠朵笉涓?鏃惰緭鍑轰腑鏂俊鍙凤紱1锛氳緭鍑轰腑鏂繀椤绘弧瓒宠缃殑鏁插嚮闃堝€间釜鏁版墠鑳借緭鍑轰腑鏂紝鍚﹀垯鏃犱腑鏂緭鍑?
        uint8_t : 3;            // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_click_crtl_t;

/*  CLICK_SRC (0x39):  鏁插嚮鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t CLICK_SRC : 4; // 鏁插嚮妫€娴嬩腑鏂姸鎬佸€?0000锛氭棤鏁插嚮浜嬩欢瑙﹀彂锛?001锛氬崟鍑讳簨浠惰Е鍙戯紱
                               // 0010锛氬弻鍑讳簨浠惰Е鍙戯紱0011锛氫笁鍑讳簨浠惰Е鍙戯紱鈥?  1111锛氬崄浜斿嚮浜嬩欢瑙﹀彂锛?
        uint8_t CLICK_SEL : 1; // 0锛氭暡鍑讳簨浠跺皬浜庣瓑浜庤缃渶澶ф暡鍑绘鏁版椂杈撳嚭涓柇
                               // 1锛氳緭鍑轰腑鏂繀椤绘弧瓒宠缃殑鏁插嚮闃堝€间釜鏁版墠鑳借緭鍑轰腑鏂紝鍚﹀垯鏃犱腑鏂緭鍑?
        uint8_t : 3;           // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_click_src_t;

/*  CLICK_COEFF1 (0x3A):  鏁插嚮绯绘暟瀵勫瓨鍣? */
typedef union {
    struct {
        uint8_t SCTH1 : 3;   // 鏁插嚮鏃惰瘑鍒湁鏁堟椂鐨勬暟鎹槇鍊?璁剧疆
        uint8_t PRE_NTH : 3; // 鏁插嚮鍓嶆暟鎹ǔ瀹氶槇鍊艰缃?
        uint8_t PRE_QT : 2;  // 鏁插嚮鍓嶆暟鎹ǔ瀹氭椂闀胯缃?
    } bit;
    uint8_t reg;
} sc7a20_click_coeff1_t;

/*  CLICK_COEFF2 (0x3B):  鏁插嚮绯绘暟瀵勫瓨鍣? */
typedef union {
    struct {
        uint8_t SCTH1T : 3; // 鏁插嚮杩囩▼涓暟鎹ぇ浜嶴CTH1闃堝€肩殑鏃堕棿涓婇檺璁剧疆
        uint8_t SCTH2 : 3;  // 鏁插嚮鏃惰瘑鍒湁鏁堟椂鐨勬暟鎹槇鍊?璁剧疆
        uint8_t QT_MT : 2;  // 鏁插嚮鍓嶅繀椤讳繚璇佹暟鎹钩绋崇殑鏈€灏忔椂闂磋缃紝闇€瑕佸ぇ浜庤闃堝€?
    } bit;
    uint8_t reg;
} sc7a20_click_coeff12_t;

/*  CLICK_COEFF3 (0x3C):  鏁插嚮绯绘暟瀵勫瓨鍣? */
typedef union {
    struct {
        uint8_t SCST : 3;  // 鏁插嚮浜嬩欢鍚庢墍鍏佽鐨勬渶澶ф仮澶嶅钩闈欐椂闀胯缃?
        uint8_t SCNTT : 2; // 婊¤冻鏁插嚮浜嬩欢鍓嶆暟鎹钩闈欐潯浠跺悗锛屽厑璁告暟鎹ぇ浜庢暟鎹櫔澹伴槇鍊肩殑鏈€澶ф椂闀?
        uint8_t : 3;       // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_click_coeff13_t;

/*  CLICK_COEFF4 (0x3D):  鏁插嚮绯绘暟瀵勫瓨鍣? */
typedef union {
    struct {
        uint8_t MCNTH : 4; // 澶氬嚮妫€娴嬫渶澶ф娴嬫鏁拌缃?
        uint8_t SCMT : 4;  // 鍗曞嚮妫€娴嬬殑鏈€澶у厑璁告椂闀?
    } bit;
    uint8_t reg;
} sc7a20_click_coeff14_t;

/*  DIG_CTRL (0x57):  鏁板瓧鍔熻兘鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t : 2;        // 淇濈暀
        uint8_t I2C_PU : 1; // SDA鍜孲CL鍐呴儴涓婃媺鐢甸樆鎺у埗浣?(绂佹涓婃媺鐢甸樆鍚庯紝璇ュ紩鑴氫负娴┖杈撳叆妯″紡锛岃淇濊瘉寮曡剼澶栧洿鐢靛钩纭畾锛屽惁鍒橧虏C閫氳浼氬紓甯?
        uint8_t SDO_PU : 1; // SDO鍐呴儴涓婃媺鐢甸樆鎺у埗浣?(绂佹涓婃媺鐢甸樆鍚庯紝璇ュ紩鑴氫负寮€婕忔ā寮忥紝璇蜂繚璇佸紩鑴氬鍥存湁涓婃媺鐢甸樆)
        uint8_t : 4;        // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_dig_ctrl_t;

/*  I2C_CTRL (0x6F):  鏁板瓧鍔熻兘鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t : 2;        // 淇濈暀
        uint8_t I2C_UN : 1; // 0锛欼IC閫氫俊浣胯兘锛?锛欼IC閫氫俊鍏抽棴锛岄厤缃?SC7A20_SOFT_RESET(68h)涓?6h鍚庯紝鍐嶉厤缃湰瀵勫瓨鍣ㄦ墠鑳界敓鏁堬紱
        uint8_t : 5;        // 淇濈暀
    } bit;
    uint8_t reg;
} sc7a20_i2c_ctrl_t;

/* SPI_CTRL (0x0E): SPI鎺у埗瀵勫瓨鍣?*/
typedef union {
    struct {
        uint8_t : 4;             // 淇濈暀浣?
        uint8_t ADR_SPI_AD6 : 1; // 鍦板潃閫夋嫨 0锛歋PI閫氫俊璁块棶鍦板潃00H~3FH锛?锛歋PI閫氫俊璁块棶鍦板潃40H~7FH
        uint8_t : 3;             // 淇濈暀浣?
    } bit;
    uint8_t reg;
} sc7a20_spi_ctrl_t;
/* ========================== 鏋氫妇绫诲瀷瀹氫箟 ========================== */
/* 鍔犻€熷害璁DR鏋氫妇 */
typedef enum {
    SC7A20_ACCEL_ODR_POWER_DOWN = 0,  // 0000: 鐢垫簮鍏虫柇妯″紡
    SC7A20_ACCEL_ODR_1_56HZ     = 1,  // 0001: 鍏ㄥ伐浣滄ā寮?1.56Hz)
    SC7A20_ACCEL_ODR_12_5HZ     = 2,  // 0010: 鍏ㄥ伐浣滄ā寮?12.5Hz)
    SC7A20_ACCEL_ODR_25HZ       = 3,  // 0011: 鍏ㄥ伐浣滄ā寮?25Hz)
    SC7A20_ACCEL_ODR_50HZ       = 4,  // 0100: 鍏ㄥ伐浣滄ā寮?50Hz)
    SC7A20_ACCEL_ODR_100HZ      = 5,  // 0101: 鍏ㄥ伐浣滄ā寮?100Hz)
    SC7A20_ACCEL_ODR_200HZ      = 6,  // 0110: 鍏ㄥ伐浣滄ā寮?200Hz)
    SC7A20_ACCEL_ODR_400HZ      = 7,  // 0111: 鍏ㄥ伐浣滄ā寮?400Hz)
    SC7A20_ACCEL_ODR_800HZ      = 8,  // 1000: 鍏ㄥ伐浣滄ā寮?800Hz)
    SC7A20_ACCEL_ODR_1_48KHZ    = 9,  // 1001: 楂樻€ц兘妯″紡(1.48kHz)
    SC7A20_ACCEL_ODR_2_66KHZ    = 10, // 1010: 楂樻€ц兘妯″紡(2.66kHz)
    SC7A20_ACCEL_ODR_4_434KHZ   = 11, // 1011: 楂樻€ц兘妯″紡(4.434kHz)
} sc7a20_accel_odr_t;

/* 鍔犻€熷害璁￠噺绋嬫灇涓?*/
typedef enum {
    SC7A20_ACCEL_FS_2G  = 0, // 卤2g
    SC7A20_ACCEL_FS_4G  = 1, // 卤4g
    SC7A20_ACCEL_FS_8G  = 2, // 卤8g
    SC7A20_ACCEL_FS_16G = 3  // 卤16g
} sc7a20_accel_fs_t;

/* fifomode 鏋氫妇 */
typedef enum {
    SC7A20_FIFO_BYPASS_MODE  = 0, // 00锛欱y-Pass妯″紡锛堟梺璺ā寮忥紝鍗充笉浣跨敤FIFO鍔熻兘锛?
    SC7A20_FIFO_FIFO_MODE    = 1, // 01:FIFO妯″紡锛堢紦瀛樻弧鏈強鏃惰鍙栵紝鏂版暟鎹涪寮冿級
    SC7A20_FIFO_STREAM_MODE  = 2, // 10:Stream妯″紡锛堢紦瀛樻弧鍚庯紝鏈€鏃╂暟鎹涪寮冿紝娣诲姞鏂版暟鎹級
    SC7A20_FIFO_TRIGGER_MODE = 3  // 11:瑙﹀彂妯″紡锛圓OI1鎴栬€匒OI2涓柇浜嬩欢鏈夋晥锛屼粠stream妯″紡杩涘叆FIFO妯″紡锛?
} sc7a20_fifo_mode_t;

#ifdef __cplusplus
}
#endif

#endif /* SC7A20_REG_H */

