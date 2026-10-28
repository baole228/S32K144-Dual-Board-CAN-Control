#include "S32K144.h" /* Thư viện định nghĩa thanh ghi chuẩn của vi điều khiển S32K144 */
#include <stdint.h>  /* Thư viện định nghĩa các kiểu dữ liệu số nguyên chuẩn */

/* ==================== ĐỊNH NGHĨA CHÂN KẾT NỐI BOARD 1 ==================== */
#define TRIG_PIN        0U    /* Chân TRIG của HC-SRF05 nối vào PTE0 (J8-05 / GPIO0) */
#define ECHO_PIN        17U   /* Chân ECHO của HC-SRF05 nối qua cầu chia áp vào PTD17 (J8-07 / GPIO1) */
#define LED_GREEN       16U   /* Đèn LED Xanh lá trên mạch MaaZEDU nối ở PTD16 (báo xe đang chạy) */
#define LED_RED         15U   /* Đèn LED Đỏ trên mạch MaaZEDU nối ở PTD15 (báo xe đang dừng) */

/* ==================== BIẾN TOÀN CỤC ==================== */
volatile uint8_t g_run_cmd = 0U;      /* Biến chốt trạng thái: 1 = Chạy liên tục, 0 = Dừng xe */
volatile uint8_t g_distance_cm = 250U;/* Biến lưu khoảng cách thực tế đo từ HC-SRF05 (cm) */

/* ==================== HÀM DELAY MICRO GIÂY & MILI GIÂY ==================== */
void delay_us(uint32_t us) {          /* Hàm tạo trễ micro giây dựa trên xung nhịp Core 48MHz */
    volatile uint32_t count = us * 4U;/* Mỗi vòng lặp tương ứng ~0.25 micro giây */
    while (count--) {                 /* Đếm giảm dần biến count về 0 */
        __asm("nop");                 /* Lệnh rỗng giữ nhịp CPU */
    }
}

void delay_ms(uint32_t ms) {          /* Hàm tạo trễ mili giây */
    while (ms--) {                    /* Lặp theo số mili giây cần chờ */
        delay_us(1000U);              /* Mỗi mili giây gọi trễ 1000 micro giây */
    }
}

/* ==================== KHỞI TẠO XUNG NHỊP (CLOCK) ==================== */
void Clock_Init(void) {               /* Hàm cấp xung nhịp FIRCDIV2 (48MHz) cho ngoại vi LPUART2 */
    SCG->FIRCDIV |= SCG_FIRCDIV_FIRCDIV2(1U); /* Chia tần số FIRC 48MHz với hệ số 1 */
}

/* ==================== KHỞI TẠO GPIO CHO SIÊU ÂM & LED ==================== */
void GPIO_Board1_Init(void) {         /* Hàm cấu hình chân GPIO cho cảm biến HC-SRF05 và đèn LED */
    PCC->PCCn[PCC_PORTD_INDEX] |= PCC_PCCn_CGC_MASK; /* Bật xung nhịp cho Port D */
    PCC->PCCn[PCC_PORTE_INDEX] |= PCC_PCCn_CGC_MASK; /* Bật xung nhịp cho Port E */

    /* Cấu hình chân TRIG (PTE0 - GPIO0) làm ngõ ra (Output) */
    PORTE->PCR[TRIG_PIN] = PORT_PCR_MUX(1U); /* Chọn chức năng GPIO (MUX = 1) cho chân PTE0 */
    PTE->PDDR |= (1UL << TRIG_PIN);          /* Đặt hướng chân PTE0 là Output để phát xung kích */
    PTE->PCOR |= (1UL << TRIG_PIN);          /* Kéo chân PTE0 xuống mức thấp (0V) ban đầu */

    /* Cấu hình chân ECHO (PTD17 - GPIO1) làm ngõ vào (Input) */
    PORTD->PCR[ECHO_PIN] = PORT_PCR_MUX(1U); /* Chọn chức năng GPIO (MUX = 1) cho chân PTD17 */
    PTD->PDDR &= ~(1UL << ECHO_PIN);         /* Đặt hướng chân PTD17 là Input để đọc xung Echo */

    /* Cấu hình LED Đỏ (PTD15) và LED Xanh lá (PTD16) trên mạch MaaZEDU */
    PORTD->PCR[LED_RED]   = PORT_PCR_MUX(1U);/* Chọn chức năng GPIO cho chân PTD15 */
    PORTD->PCR[LED_GREEN] = PORT_PCR_MUX(1U);/* Chọn chức năng GPIO cho chân PTD16 */
    PTD->PDDR |= (1UL << LED_RED) | (1UL << LED_GREEN); /* Đặt PTD15 và PTD16 làm Output */
    PTD->PSOR |= (1UL << LED_GREEN);         /* Tắt LED Xanh lá ban đầu */
    PTD->PCOR |= (1UL << LED_RED);           /* Bật LED Đỏ ban đầu báo hiệu xe đang dừng đợi lệnh */
}

/* ==================== KHỞI TẠO LPUART2 CHO BLUETOOTH HC-05 ==================== */
void LPUART2_Init(void) {             /* Hàm khởi tạo UART2 ở tốc độ 9600 bps (Chế độ chỉ nhận chống nhiễu HC-05) */
    PCC->PCCn[PCC_PORTD_INDEX] |= PCC_PCCn_CGC_MASK; /* Đảm bảo Port D đã được cấp xung nhịp */
    PORTD->PCR[6] = PORT_PCR_MUX(2U); /* Cấu hình PTD6 (J8-16) làm LPUART2_RX (nối với chân TX của HC-05) */

    PCC->PCCn[PCC_LPUART2_INDEX] &= ~PCC_PCCn_CGC_MASK;     /* Tắt clock LPUART2 trước khi chọn nguồn xung */
    PCC->PCCn[PCC_LPUART2_INDEX] |= PCC_PCCn_PCS(3U)        /* Chọn nguồn xung PCS = 3 (FIRCDIV2_CLK = 48MHz) */
                                 |  PCC_PCCn_CGC_MASK;      /* Bật lại clock cho LPUART2 */

    LPUART2->BAUD = LPUART_BAUD_OSR(24U)     /* Tỷ lệ lấy mẫu OSR = 24 (25 lần lấy mẫu/bit) */
                  | LPUART_BAUD_SBR(200U);   /* Hệ số chia SBR = 200 -> Baudrate = 9600 bps */

    LPUART2->CTRL = LPUART_CTRL_RE_MASK;     /* Chỉ bật bộ nhận (RE) để giữ kết nối HC-05 ổn định tuyệt đối */
}

/* ==================== HÀM ĐỌC LỆNH BLUETOOTH (CHỐT TRẠNG THÁI CHẠY/DỪNG) ==================== */
void Bluetooth_CheckCommand(void) {   /* Hàm đọc ký tự từ App điện thoại và chốt lệnh điều khiển */
    /* Xóa mọi cờ lỗi UART (Overrun, Noise, Framing) nếu có để bộ nhận không bao giờ bị kẹt */
    if (LPUART2->STAT & (LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK)) {
        LPUART2->STAT |= (LPUART_STAT_OR_MASK | LPUART_STAT_NF_MASK | LPUART_STAT_FE_MASK);
    }

    if (LPUART2->STAT & LPUART_STAT_RDRF_MASK) { /* Kiểm tra cờ RDRF xem có ký tự mới từ HC-05 gửi vào không */
        char cmd = (char)(LPUART2->DATA & 0xFFU);/* Đọc ký tự nhận được từ thanh ghi DATA */

        /* Nhấn nút TIẾN 1 lần ('F', 'f' hoặc '1') -> Chốt g_run_cmd = 1 cho xe chạy liên tục */
        if (cmd == 'F' || cmd == 'f' || cmd == '1') {
            g_run_cmd = 1U;                      /* Chốt trạng thái chạy tiến không dừng */
            PTD->PCOR |= (1UL << LED_GREEN);     /* Bật sáng LED Xanh lá trên Board 1 */
            PTD->PSOR |= (1UL << LED_RED);       /* Tắt LED Đỏ trên Board 1 */
        }
        /* Nhấn nút LÙI 1 lần ('B', 'b' hoặc '2') -> Chốt g_run_cmd = 0 để dừng xe ngay lập tức */
        else if (cmd == 'B' || cmd == 'b' || cmd == '2') {
            g_run_cmd = 0U;                      /* Chốt trạng thái dừng xe */
            PTD->PSOR |= (1UL << LED_GREEN);     /* Tắt LED Xanh lá trên Board 1 */
            PTD->PCOR |= (1UL << LED_RED);       /* Bật sáng LED Đỏ báo hiệu xe dừng */
        }
    }
}

/* ==================== HÀM ĐO KHOẢNG CÁCH CẢM BIẾN SIÊU ÂM HC-SRF05 ==================== */
uint8_t HCSRF05_GetDistance(void) {   /* Hàm phát xung Trigger và đo khoảng cách trả về bằng cm */
    uint32_t timeout = 0U;            /* Biến đếm chống treo khi không có sóng phản xạ */
    uint32_t pulse_ticks = 0U;        /* Biến đếm độ rộng xung mức cao trên chân ECHO */

    /* Phát xung kích 12 micro giây trên chân TRIG (PTE0) */
    PTE->PCOR |= (1UL << TRIG_PIN);   /* Đưa chân TRIG về mức 0V */
    delay_us(2U);                     /* Chờ ổn định 2 micro giây */
    PTE->PSOR |= (1UL << TRIG_PIN);   /* Kéo chân TRIG lên mức cao 3.3V */
    delay_us(12U);                    /* Giữ mức cao trong 12 micro giây */
    PTE->PCOR |= (1UL << TRIG_PIN);   /* Hạ chân TRIG xuống 0V kết thúc xung kích */

    /* Đợi chân ECHO (PTD17) chuyển lên mức cao */
    timeout = 30000U;                 /* Giới hạn thời gian đợi sườn lên */
    while (((PTD->PDIR & (1UL << ECHO_PIN)) == 0U) && (timeout > 0U)) {
        timeout--;                    /* Giảm biến đếm chờ */
    }
    if (timeout == 0U) {              /* Nếu quá thời gian mà ECHO không lên mức 1 */
        return 250U;                  /* Trả về khoảng cách xa an toàn (250 cm) */
    }

    /* Đếm thời gian chân ECHO duy trì ở mức cao */
    timeout = 60000U;                 /* Giới hạn thời gian đo tối đa */
    while (((PTD->PDIR & (1UL << ECHO_PIN)) != 0U) && (timeout > 0U)) {
        pulse_ticks++;                /* Tăng biến đếm độ rộng xung */
        delay_us(1U);                 /* Trễ 1 micro giây mỗi nhịp */
        timeout--;                    /* Giảm biến timeout */
    }

    /* Quy đổi độ rộng xung sang khoảng cách (đơn vị: cm) */
    uint32_t dist = (pulse_ticks * 135U) / 5800U; /* Tính khoảng cách dựa trên tốc độ âm thanh */
    if (dist > 250U || dist == 0U) {  /* Nếu vượt ngưỡng 250cm hoặc bằng 0 do nhiễu */
        dist = 250U;                  /* Gán về giá trị an toàn 250cm */
    }
    return (uint8_t)dist;             /* Trả về khoảng cách từ 1 đến 250 cm */
}

/* ==================== KHỞI TẠO GIAO THỨC FLEXCAN0 (500 KBPS) ==================== */
void FlexCAN0_Init(void) {            /* Hàm cấu hình CAN0 trên PTE4 (RX) và PTE5 (TX) */
    PCC->PCCn[PCC_PORTE_INDEX] |= PCC_PCCn_CGC_MASK; /* Cấp xung nhịp cho Port E */
    PORTE->PCR[4] = PORT_PCR_MUX(5U); /* Chọn chức năng CAN0_RX (MUX = 5) cho chân PTE4 */
    PORTE->PCR[5] = PORT_PCR_MUX(5U); /* Chọn chức năng CAN0_TX (MUX = 5) cho chân PTE5 */

    PCC->PCCn[PCC_FlexCAN0_INDEX] |= PCC_PCCn_CGC_MASK; /* Cấp xung nhịp cho mô-đun FlexCAN0 */

    CAN0->MCR |= CAN_MCR_MDIS_MASK;   /* Tắt mô-đun CAN tạm thời để chọn nguồn xung nhịp */
    CAN0->CTRL1 |= CAN_CTRL1_CLKSRC_MASK; /* Chọn nguồn xung nhịp Peripheral Clock (48MHz) */
    CAN0->MCR &= ~CAN_MCR_MDIS_MASK;  /* Bật lại mô-đun CAN */

    while (!(CAN0->MCR & CAN_MCR_FRZACK_MASK)) {} /* Đợi CAN0 vào chế độ Freeze Mode */

    /* Thiết lập tốc độ truyền 500 kbps từ nguồn 48MHz */
    CAN0->CTRL1 = CAN_CTRL1_CLKSRC_MASK
                | CAN_CTRL1_PRESDIV(5U)
                | CAN_CTRL1_RJW(3U)
                | CAN_CTRL1_PSEG1(4U)
                | CAN_CTRL1_PSEG2(4U)
                | CAN_CTRL1_PROPSEG(4U);

    for (uint32_t i = 0U; i < 128U; i++) {
        CAN0->RAMn[i] = 0U;           /* Xóa sạch bộ nhớ RAM của các Message Buffers */
    }

    CAN0->RAMn[0 * 4 + 0] = (0x8UL << 24); /* Đặt Message Buffer 0 (MB0) ở trạng thái chờ truyền (TX Inactive) */

    CAN0->MCR = 0x0000001FU;          /* Thoát chế độ Freeze Mode và bật 32 Message Buffers */
    while (CAN0->MCR & CAN_MCR_FRZACK_MASK) {} /* Chờ thoát hoàn toàn khỏi Freeze Mode */
    while (CAN0->MCR & CAN_MCR_NOTRDY_MASK) {} /* Chờ bus CAN sẵn sàng hoạt động */
}

/* ==================== HÀM TRUYỀN GÓI TIN CAN SANG BOARD 2 ==================== */
void FlexCAN0_SendStatus(uint8_t run_cmd, uint8_t dist_cm) { /* Đóng gói và truyền 2 byte qua CAN ID 0x100 */
    CAN0->IFLAG1 = (1UL << 0);        /* Xóa cờ truyền của Message Buffer 0 (MB0) */

    /* Ghi dữ liệu vào Word 2 của MB0: Byte 0 là lệnh chạy/dừng, Byte 1 là khoảng cách siêu âm */
    CAN0->RAMn[0 * 4 + 2] = ((uint32_t)run_cmd << 24)  /* Đưa run_cmd vào Byte 0 [31:24] */
                          | ((uint32_t)dist_cm << 16); /* Đưa dist_cm vào Byte 1 [23:16] */
    CAN0->RAMn[0 * 4 + 3] = 0U;       /* Xóa Word 3 về 0 */

    CAN0->RAMn[0 * 4 + 1] = (0x100UL << 18); /* Thiết lập Standard ID = 0x100 */

    /* Kích hoạt truyền: CODE = 0xC (TX Data Frame), SRR = 1, DLC = 2 bytes */
    CAN0->RAMn[0 * 4 + 0] = (0xCUL << 24)
                          | (1UL << 22)
                          | (2UL << 16);
}

/* ==================== HÀM MAIN CHÍNH CỦA BOARD 1 ==================== */
int main(void) {                      /* Điểm bắt đầu chương trình của Board 1 */
    Clock_Init();                     /* Khởi tạo xung nhịp 48MHz cho LPUART2 */
    GPIO_Board1_Init();               /* Khởi tạo chân TRIG, ECHO và đèn LED trạng thái */
    LPUART2_Init();                   /* Khởi tạo UART2 giao tiếp với Bluetooth HC-05 */
    FlexCAN0_Init();                  /* Khởi tạo giao thức CAN0 tốc độ 500 kbps */

    while (1) {                       /* Vòng lặp chính chạy liên tục */
        Bluetooth_CheckCommand();     /* Kiểm tra lệnh Tiến/Lùi từ điện thoại */
        g_distance_cm = HCSRF05_GetDistance(); /* Đo khoảng cách vật cản thực tế bằng HC-SRF05 */
        FlexCAN0_SendStatus(g_run_cmd, g_distance_cm); /* Truyền trạng thái chạy và khoảng cách sang Board 2 */

        /* Lặp chờ 30ms nhưng kiểm tra UART mỗi 1ms để bắt nút nhấn Bluetooth nhạy tức thì */
        for (uint8_t i = 0U; i < 30U; i++) {
            Bluetooth_CheckCommand(); /* Liên tục quét dữ liệu từ HC-05 */
            delay_ms(1U);             /* Trễ 1ms */
        }
    }
    return 0;                         /* Kết thúc hàm main */
}
