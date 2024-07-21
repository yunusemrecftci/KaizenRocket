#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IDC_TEAMID 104
#define IDC_SET_TEAMID_BUTTON 105
#define IDC_COMPORT_RECIEVE 101
#define IDC_BAUDRATE_RECIEVE 102
#define IDC_COMPORT_SEND 103
#define IDC_BAUDRATE_SEND 104
#define IDC_REFRESH_BUTTON 105
#define IDC_SET_PARAMETERS_BUTTON 105
/*
yapılacaklar:

-şu an ilk başta com açılmaya çalışılıyor ilk kullanıcıdan hangi portu açmak istediği sorulcak sonra açılcak

-verilerin gelme sıklığı kontrol edilecek

-gelen veriler gerekli struct içine yerleştirilecek

-gelen veri - kullanıcalak veri - giden veri strucları düzenlenecek

-refresh ile veriler yenileniyor -> refreshin içindeki parametre atama kısmı başka fonksiyona atılacak refresh her veri geldiğinde çağırılacak

-süre tutulacak buna göre veri gönderme çağıralacak

-ui değişecek

-tüm parçalar test edilecek

*** tek threat olacak main fonsiyonda tutukuluk olmaması için while döngüleri içinde bekleme olmamalı***

*/
int run = 0;
typedef struct {
    unsigned char id;
    float temperature;
    float humidity;
} LoRaData1;

typedef struct {
    unsigned char id;
    float pressure;
    float altitude;
} LoRaData2;

typedef struct {
    LoRaData1 data1;
    LoRaData2 data2;
} CombinedData;

void mainreciever();
void paket_olustur();
void portopentosend();
int read_from_serial_port(HANDLE hSerial, unsigned char *data);
void process_lora_data(unsigned char data1, unsigned char data2, CombinedData *combinedData);
char comrecieve[5];
char comsend[5];
int baudrecieve;
int baudsend;

//UI
HWND hwndComPortTextBoxRecieve, hwndBaudRateTextBoxRecieve;
HWND hwndComPortTextBoxSend, hwndBaudRateTextBoxSend;
HWND hwndRefreshButton, hwndHeightDisplay, hwndVariableDisplay[15];
HWND hwndTeamIDTextBox,hwndSetParametersButton;
char height[10] = "****";
char variableValues[15][10];

void RefreshValues();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
case WM_DESTROY:{
            PostQuitMessage(0);
return 0;}
case WM_CREATE: {
    int windowWidth = 800;
    int xOffset = windowWidth - 220;

    // COM port (Recieve) etiketi
    CreateWindow("STATIC", "Enter COM Port (Recieve):",
                 WS_CHILD | WS_VISIBLE,
                 xOffset, 10, 200, 20,
                 hwnd, NULL, NULL, NULL);

    // COM port (Recieve) metin kutusu
    hwndComPortTextBoxRecieve = CreateWindow("EDIT", "",
                                             WS_CHILD | WS_VISIBLE | WS_BORDER,
                                             xOffset, 30, 200, 20,
                                             hwnd, (HMENU) IDC_COMPORT_RECIEVE, NULL, NULL);

    // Baud rate (Recieve) etiketi
    CreateWindow("STATIC", "Enter Baud Rate (Recieve):",
                 WS_CHILD | WS_VISIBLE,
                 xOffset, 60, 200, 20,
                 hwnd, NULL, NULL, NULL);

    // Baud rate (Recieve) metin kutusu
    hwndBaudRateTextBoxRecieve = CreateWindow("EDIT", "",
                                              WS_CHILD | WS_VISIBLE | WS_BORDER,
                                              xOffset, 80, 200, 20,
                                              hwnd, (HMENU) IDC_BAUDRATE_RECIEVE, NULL, NULL);

    // COM port (Send) etiketi
    CreateWindow("STATIC", "Enter COM Port (Send):",
                 WS_CHILD | WS_VISIBLE,
                 xOffset, 110, 200, 20,
                 hwnd, NULL, NULL, NULL);

    // COM port (Send) metin kutusu
    hwndComPortTextBoxSend = CreateWindow("EDIT", "",
                                          WS_CHILD | WS_VISIBLE | WS_BORDER,
                                          xOffset, 130, 200, 20,
                                          hwnd, (HMENU) IDC_COMPORT_SEND, NULL, NULL);

    // Baud rate (Send) etiketi
    CreateWindow("STATIC", "Enter Baud Rate (Send):",
                 WS_CHILD | WS_VISIBLE,
                 xOffset, 160, 200, 20,
                 hwnd, NULL, NULL, NULL);

    // Baud rate (Send) metin kutusu
    hwndBaudRateTextBoxSend = CreateWindow("EDIT", "",
                                           WS_CHILD | WS_VISIBLE | WS_BORDER,
                                           xOffset, 180, 200, 20,
                                           hwnd, (HMENU) IDC_BAUDRATE_SEND, NULL, NULL);

    // Team ID etiketi
    CreateWindow("STATIC", "Enter Team ID:",
                 WS_CHILD | WS_VISIBLE,
                 xOffset, 210, 200, 20,
                 hwnd, NULL, NULL, NULL);

    // Team ID metin kutusu
    hwndTeamIDTextBox = CreateWindow("EDIT", "",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     xOffset, 230, 200, 20,
                                     hwnd, (HMENU) IDC_TEAMID, NULL, NULL);

    // Set Parameters butonu
    hwndSetParametersButton = CreateWindow("BUTTON", "Set Parameters",
                                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                           xOffset, 260, 200, 30,
                                           hwnd, (HMENU) IDC_SET_PARAMETERS_BUTTON, NULL, NULL);

    // Height için gösterge
    hwndHeightDisplay = CreateWindow("STATIC", "Height - ****",
                                     WS_CHILD | WS_VISIBLE,
                                     10, 10, 200, 20,
                                     hwnd, NULL, NULL, NULL);

    // Diğer değişkenler için göstergeler
    for (int i = 0; i < 15; i++) {
        hwndVariableDisplay[i] = CreateWindow("STATIC", "Variable - ****",
                                              WS_CHILD | WS_VISIBLE,
                                              10, 40 + i * 20, 200, 20,
                                              hwnd, NULL, NULL, NULL);
    }
    return 0;
}

case WM_COMMAND: {
    if (LOWORD(wParam) == IDC_SET_PARAMETERS_BUTTON) {
        char comPortTextRecieve[100], baudRateTextRecieve[100];
        char comPortTextSend[100], baudRateTextSend[100];
        char teamIDText[100];

        GetWindowText(hwndComPortTextBoxRecieve, comPortTextRecieve, 100);
        GetWindowText(hwndBaudRateTextBoxRecieve, baudRateTextRecieve, 100);
        GetWindowText(hwndComPortTextBoxSend, comPortTextSend, 100);
        GetWindowText(hwndBaudRateTextBoxSend, baudRateTextSend, 100);
        GetWindowText(hwndTeamIDTextBox, teamIDText, 100);

        int baudRateRecieve = atoi(baudRateTextRecieve);
        int baudRateSend = atoi(baudRateTextSend);
		baudrecieve = baudRateRecieve;
		baudsend = baudRateSend;
		strcpy(comrecieve,comPortTextRecieve);
		strcpy(comsend,comPortTextSend);
		
        char message[500];
        sprintf(message, "COM Port (Recieve): %s\nBaud Rate (Recieve): %d\nCOM Port (Send): %s\nBaud Rate (Send): %d\nTeam ID: %s",
                comPortTextRecieve, baudRateRecieve, comPortTextSend, baudRateSend, teamIDText);
        MessageBox(hwnd, message, "Parameters Set", MB_OK);

        // Diğer işlemler burada yapılabilir
        run = 1;
        RefreshValues();
        
    }
    return 0;
}

		default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void RefreshValues() {
    sprintf(height, "%d", rand() % 100);
    char displayText[50];
    sprintf(displayText, "Height - %s", height);
    SetWindowText(hwndHeightDisplay, displayText);

    for (int i = 0; i < 15; i++) {
        sprintf(variableValues[i], "%d", rand() % 1000);
        sprintf(displayText, "Variable %d - %s", i + 1, variableValues[i]);
        SetWindowText(hwndVariableDisplay[i], displayText);
    }
}
//program main fonksiyonu
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "COM Port and Baud Rate Setter", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    mainreciever();
    return 0;
}

typedef union {
    float sayi;
    unsigned char array[4];
} FLOAT32_UINT8_DONUSTURUCU;

unsigned char olusturalacak_paket[78];

unsigned char check_sum_hesapla() {
    int check_sum = 0;
    for (int i = 4; i < 75; i++) {
        check_sum += olusturalacak_paket[i];
    }
    return (unsigned char) (check_sum % 256);
}

void paket_olustur() {
    olusturalacak_paket[0] = 0xFF;
    olusturalacak_paket[1] = 0xFF;
    olusturalacak_paket[2] = 0x54;
    olusturalacak_paket[3] = 0x52;
    olusturalacak_paket[4] = 0;
    olusturalacak_paket[5] = 0;

    FLOAT32_UINT8_DONUSTURUCU irtifa_float32_uint8_donusturucu;
    irtifa_float32_uint8_donusturucu.sayi = 10.2;
    olusturalacak_paket[6] = irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[7] = irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[8] = irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[9] = irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU roket_gps_irtifa_float32_uint8_donusturucu;
    roket_gps_irtifa_float32_uint8_donusturucu.sayi = 1461.55;
    olusturalacak_paket[10] = roket_gps_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[11] = roket_gps_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[12] = roket_gps_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[13] = roket_gps_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU roket_enlem_float32_uint8_donusturucu;
    roket_enlem_float32_uint8_donusturucu.sayi = 39.925019;
    olusturalacak_paket[14] = roket_enlem_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[15] = roket_enlem_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[16] = roket_enlem_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[17] = roket_enlem_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU roket_boylam_irtifa_float32_uint8_donusturucu;
    roket_boylam_irtifa_float32_uint8_donusturucu.sayi = 32.836954;
    olusturalacak_paket[18] = roket_boylam_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[19] = roket_boylam_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[20] = roket_boylam_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[21] = roket_boylam_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU gorev_yuku_gps_irtifa_float32_uint8_donusturucu;
    gorev_yuku_gps_irtifa_float32_uint8_donusturucu.sayi = 1361.61;
    olusturalacak_paket[22] = gorev_yuku_gps_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[23] = gorev_yuku_gps_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[24] = gorev_yuku_gps_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[25] = gorev_yuku_gps_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU gorev_yuku_enlem_float32_uint8_donusturucu;
    gorev_yuku_enlem_float32_uint8_donusturucu.sayi = 41.104593;
    olusturalacak_paket[26] = gorev_yuku_enlem_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[27] = gorev_yuku_enlem_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[28] = gorev_yuku_enlem_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[29] = gorev_yuku_enlem_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU gorev_yuku_boylam_irtifa_float32_uint8_donusturucu;
    gorev_yuku_boylam_irtifa_float32_uint8_donusturucu.sayi = 29.024411;
    olusturalacak_paket[30] = gorev_yuku_boylam_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[31] = gorev_yuku_boylam_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[32] = gorev_yuku_boylam_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[33] = gorev_yuku_boylam_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU kademe_gps_irtifa_float32_uint8_donusturucu;
    kademe_gps_irtifa_float32_uint8_donusturucu.sayi = 1666.61;
    olusturalacak_paket[34] = kademe_gps_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[35] = kademe_gps_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[36] = kademe_gps_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[37] = kademe_gps_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU kademe_enlem_float32_uint8_donusturucu;
    kademe_enlem_float32_uint8_donusturucu.sayi = 41.091485;
    olusturalacak_paket[38] = kademe_enlem_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[39] = kademe_enlem_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[40] = kademe_enlem_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[41] = kademe_enlem_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU kademe_boylam_irtifa_float32_uint8_donusturucu;
    kademe_boylam_irtifa_float32_uint8_donusturucu.sayi = 29.061412;
    olusturalacak_paket[42] = kademe_boylam_irtifa_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[43] = kademe_boylam_irtifa_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[44] = kademe_boylam_irtifa_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[45] = kademe_boylam_irtifa_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU jiroskop_x_float32_uint8_donusturucu;
    jiroskop_x_float32_uint8_donusturucu.sayi = 1.51;
    olusturalacak_paket[46] = jiroskop_x_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[47] = jiroskop_x_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[48] = jiroskop_x_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[49] = jiroskop_x_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU jiroskop_y_float32_uint8_donusturucu;
    jiroskop_y_float32_uint8_donusturucu.sayi = 0.49;
    olusturalacak_paket[50] = jiroskop_y_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[51] = jiroskop_y_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[52] = jiroskop_y_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[53] = jiroskop_y_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU jiroskop_z_float32_uint8_donusturucu;
    jiroskop_z_float32_uint8_donusturucu.sayi = 0.61;
    olusturalacak_paket[54] = jiroskop_z_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[55] = jiroskop_z_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[56] = jiroskop_z_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[57] = jiroskop_z_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU ivme_x_float32_uint8_donusturucu;
    ivme_x_float32_uint8_donusturucu.sayi = 0.0411;
    olusturalacak_paket[58] = ivme_x_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[59] = ivme_x_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[60] = ivme_x_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[61] = ivme_x_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU ivme_y_float32_uint8_donusturucu;
    ivme_y_float32_uint8_donusturucu.sayi = 0.0140;
    olusturalacak_paket[62] = ivme_y_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[63] = ivme_y_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[64] = ivme_y_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[65] = ivme_y_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU ivme_z_float32_uint8_donusturucu;
    ivme_z_float32_uint8_donusturucu.sayi = -0.9552;
    olusturalacak_paket[66] = ivme_z_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[67] = ivme_z_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[68] = ivme_z_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[69] = ivme_z_float32_uint8_donusturucu.array[3];

    FLOAT32_UINT8_DONUSTURUCU aci_float32_uint8_donusturucu;
    aci_float32_uint8_donusturucu.sayi = 5.08;
    olusturalacak_paket[70] = aci_float32_uint8_donusturucu.array[0];
    olusturalacak_paket[71] = aci_float32_uint8_donusturucu.array[1];
    olusturalacak_paket[72] = aci_float32_uint8_donusturucu.array[2];
    olusturalacak_paket[73] = aci_float32_uint8_donusturucu.array[3];

    olusturalacak_paket[74] = 1;
    olusturalacak_paket[75] = check_sum_hesapla();
    olusturalacak_paket[76] = 0x0D;
    olusturalacak_paket[77] = 0x0A;
}

void portopentosend() {
    HANDLE hSerial = CreateFile("COM7", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        perror("Unable to open COM send");
        return;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        perror("GetCommState error");
        CloseHandle(hSerial);
        return;
    }
    if(baudrecieve == 2400){dcbSerialParams.BaudRate = CBR_2400;}
    else if(baudrecieve == 9600){dcbSerialParams.BaudRate = CBR_9600;}
    else if(baudrecieve == 19200){dcbSerialParams.BaudRate = CBR_19200;}
    else{dcbSerialParams.BaudRate = CBR_9600;}
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        perror("SetCommState error");
        CloseHandle(hSerial);
        return;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        perror("SetCommTimeouts error");
        CloseHandle(hSerial);
        return;
    }

    DWORD bytes_written;
    if (!WriteFile(hSerial, olusturalacak_paket, sizeof(olusturalacak_paket), &bytes_written, NULL)) {
        perror("WriteFile error");
    } else {
        printf("Veri gönderildi: %d byte\n", bytes_written);
    }
}

int read_from_serial_port(HANDLE hSerial, unsigned char *data) {
    DWORD bytesRead;
    if (!ReadFile(hSerial, data, 1, &bytesRead, NULL)) {
        return -1;
    }
    return bytesRead;
}

void process_lora_data(unsigned char data1, unsigned char data2, CombinedData *combinedData) {
    combinedData->data1.id = data1 >> 4;
    combinedData->data1.temperature = (data1 & 0x0F) * 10.0f;
    combinedData->data1.humidity = (float)data1 / 2.0f;

    combinedData->data2.id = data2 >> 4;
    combinedData->data2.pressure = (data2 & 0x0F) * 100.0f;
    combinedData->data2.altitude = (float)data2 / 4.0f;
}

void mainreciever() {
if(run){
    HANDLE hSerial;
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts = {0};
    unsigned char lora_data1, lora_data2;
    CombinedData combinedData;

    hSerial = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Seri port açma başarısız\n");
        return;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Seri port ayarları alınamadı\n");
        return;
    }

    if(baudrecieve == 2400){dcbSerialParams.BaudRate = CBR_2400;}
    else if(baudrecieve == 9600){dcbSerialParams.BaudRate = CBR_9600;}
    else if(baudrecieve == 19200){dcbSerialParams.BaudRate = CBR_19200;}
    else{dcbSerialParams.BaudRate = CBR_9600;}
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        fprintf(stderr, "Seri port ayarları uygulanamadı\n");
        return;
    }

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        fprintf(stderr, "Seri port zaman aşımı ayarları uygulanamadı\n");
        return;
    }

    while (1) {
        if (read_from_serial_port(hSerial, &lora_data1) > 0 &&
            read_from_serial_port(hSerial, &lora_data2) > 0) {
            process_lora_data(lora_data1, lora_data2, &combinedData);

            printf("LoRa Modülü 1 - ID: %d, Sıcaklık: %.2f, Nem: %.2f\n", 
                    combinedData.data1.id, combinedData.data1.temperature, combinedData.data1.humidity);
            printf("LoRa Modülü 2 - ID: %d, Basınç: %.2f, Rakım: %.2f\n", 
                    combinedData.data2.id, combinedData.data2.pressure, combinedData.data2.altitude);
        }

        Sleep(100);
    }
    }
	else{
		Sleep(5);
	}
	return;
}
