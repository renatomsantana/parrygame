/*
 * entrada_win.c - carimbos no Windows. Uma thread própria, com uma janela só de mensagens, pede ao sistema
 * a entrada bruta (Raw Input: botão do mouse e teclado, direto do dispositivo, venha de onde vier o foco) e
 * marca com o QueryPerformanceCounter o instante em que cada WM_INPUT chega: uns décimos de milissegundo
 * depois do hardware, e sem esperar o poll do GLFW, que só acontece depois do swap do vsync. Só o botão
 * esquerdo (o que o sistema chama de primário, se o mouse estiver trocado para canhotos) e as teclas
 * Espaço, J e Enter contam (as mesmas de pressed() no main.c); a repetição de tecla não. Quem decide se o
 * carimbo pertence a um aperto que o jogo leu é o main.c: o carimbo de um clique que a janela não recebeu
 * (fora de foco) nunca é usado. Qualquer falha (sem a thread, sem a janela, sem o Raw Input) devolve false
 * em entrada_iniciar, e todo aperto cai no meio do quadro, como no entrada_stub.c.
 *
 * Não roda no Wine sem um servidor de entrada de verdade: o teste da parte pura (a fila, a conversão) é o
 * entrada_test, e este arquivo é conferido pelo MinGW (make teste-windows). No Windows, o jeito de ver se
 * funciona é `apara --carimbo`: o atraso do poll em relação ao clique tem de dar uns milissegundos, e não 0.
 */
#include "entrada_plat.h"

#include "entrada_fila.h"

#include <windows.h>

static EntradaFila fila;
static CRITICAL_SECTION trava;
static volatile LONG iniciada;      /* a trava já foi criada */
static volatile LONG estado;        /* 0 começando, 1 recebendo, -1 sem janela ou sem Raw Input */
static LARGE_INTEGER frequencia;

double entrada_relogio(void) {
    LARGE_INTEGER c;
    if (frequencia.QuadPart == 0) QueryPerformanceFrequency(&frequencia);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)frequencia.QuadPart;
}

static void empilha(double t) {
    EnterCriticalSection(&trava);
    fila_empilha(&fila, t);
    LeaveCriticalSection(&trava);
}

/* as teclas de aparar (Espaço, J e Enter), e as que já estão apertadas */
static bool tecla_de_aparar(USHORT vk) { return vk == VK_SPACE || vk == 'J' || vk == VK_RETURN; }
static EntradaTeclas teclas;

static LRESULT CALLBACK proc_janela(HWND janela, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_INPUT) {
        const double t = entrada_relogio();
        RAWINPUT ri;
        UINT n = sizeof ri;
        if (GetRawInputData((HRAWINPUT)l, RID_INPUT, &ri, &n, sizeof(RAWINPUTHEADER)) != (UINT)-1) {
            if (ri.header.dwType == RIM_TYPEMOUSE) {
                const USHORT primario = GetSystemMetrics(SM_SWAPBUTTON) ? RI_MOUSE_RIGHT_BUTTON_DOWN : RI_MOUSE_LEFT_BUTTON_DOWN;
                if (ri.data.mouse.usButtonFlags & primario) empilha(t);
            } else if (ri.header.dwType == RIM_TYPEKEYBOARD) {
                const USHORT vk = ri.data.keyboard.VKey;
                if (teclas_aperta(&teclas, vk, (ri.data.keyboard.Flags & RI_KEY_BREAK) != 0) && tecla_de_aparar(vk)) empilha(t);
            }
        }
        return DefWindowProcW(janela, msg, w, l);   /* o sistema pede esta chamada em todo WM_INPUT */
    }
    return DefWindowProcW(janela, msg, w, l);
}

static DWORD WINAPI le_eventos(LPVOID arg) {
    (void)arg;
    const wchar_t *classe = L"AparaEntradaBruta";
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = proc_janela;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = classe;
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) { estado = -1; return 0; }
    HWND janela = CreateWindowExW(0, classe, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
    if (!janela) { estado = -1; return 0; }
    RAWINPUTDEVICE dispositivos[2];
    dispositivos[0].usUsagePage = 0x01;     /* desktop genérico */
    dispositivos[0].usUsage = 0x02;         /* mouse */
    dispositivos[0].dwFlags = RIDEV_INPUTSINK;      /* recebe também sem o foco: o main.c decide o que vale */
    dispositivos[0].hwndTarget = janela;
    dispositivos[1].usUsagePage = 0x01;
    dispositivos[1].usUsage = 0x06;         /* teclado */
    dispositivos[1].dwFlags = RIDEV_INPUTSINK;
    dispositivos[1].hwndTarget = janela;
    if (!RegisterRawInputDevices(dispositivos, 2, sizeof dispositivos[0])) { DestroyWindow(janela); estado = -1; return 0; }
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);   /* acorda logo quando o WM_INPUT chega */
    estado = 1;
    MSG m;
    while (GetMessageW(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}

void entrada_preparar(void) {
    if (InterlockedCompareExchange(&iniciada, 1, 0) == 0) InitializeCriticalSection(&trava);
    QueryPerformanceFrequency(&frequencia);
}

/* Espera a thread dizer se conseguiu (o jogo não abre mais devagar por isso: são uns milissegundos). */
bool entrada_iniciar(void) {
    entrada_preparar();
    HANDLE h = CreateThread(NULL, 0, le_eventos, NULL, 0, NULL);
    if (!h) return false;
    CloseHandle(h);
    for (int i = 0; i < 500 && estado == 0; i++) Sleep(1);
    return estado == 1;
}

int entrada_coletar(double *carimbos, int max, double ate) {
    if (!iniciada) return 0;
    EnterCriticalSection(&trava);
    int n = fila_coleta(&fila, carimbos, max, ate);
    LeaveCriticalSection(&trava);
    return n;
}
