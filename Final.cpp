#include <Keypad.h>
#include <LiquidCrystal.h>
#include <MFRC522.h>
#include <SD.h>

const char vermelho = 41;
const char verde = 40;
const char ambar = 42;
const char linhas = 4;
const char colunas = 4;
const char separador = 46;
const int limite = 5;
const int maximo = 10;
const String administrador = "00000";
const String operador = "99999";
const String base = "BASE.txt";
const char mapaTeclas[linhas][colunas] = {{'1','2','3','A'},
                                          {'4','5','6','B'},
                                          {'7','8','9','C'},
                                          {'*','0','#','D'}};

byte pinosLinha[linhas] = {26, 27, 28, 29};
byte pinosColuna[colunas] = {22, 23, 24, 25};
Keypad teclado = Keypad(makeKeymap(mapaTeclas), pinosLinha, pinosColuna, linhas, colunas);
LiquidCrystal lcd(32, 33, 34, 35, 36, 37);
MFRC522 rfid(53, 2);

struct Aluno {
    String registro;
    String senha;
    String cartao;
} alunos[maximo];

int pronto = -1;
int atual = 0;


void setup() {

    pinMode(vermelho, OUTPUT);
    pinMode(verde, OUTPUT);
    pinMode(ambar, OUTPUT);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);
    if ((SD.begin(4)) && (carga() == 0)) {
        pronto = 0;
        digitalWrite(vermelho, HIGH);
        digitalWrite(verde, HIGH);
        digitalWrite(ambar, HIGH);
        lcd.print("**   PRONTO   **");
        lcd.setCursor(0, 1);
        lcd.print("**  PARA USO  **");
        delay(2000);
        digitalWrite(vermelho, LOW);
        digitalWrite(verde, LOW);
        digitalWrite(ambar, LOW);
        menuPrincipal();
    }
    else {
        lcd.print("** INOPERANTE **");
        lcd.setCursor(0, 1);
        lcd.print("** ERRO GERAL **");
    }

}


void loop() {

    char tecla = teclado.waitForKey();
    if ((pronto == 0) && ((tecla == '*') || (tecla == '#') || (tecla == '0'))) {
        if (tecla == '*')
            acessoTeclado();
        else if (tecla == '#')
            acessoCartao();
        else
            menuAdministrador();
        menuPrincipal();
    }

}


int carga() {

    return (((SD.exists(base)) && (carregaBase() == 0)) || (criaBase() == 0)) ? 0 : -1;

}


int carregaBase() {

    int status = -1;
    File arquivo = SD.open(base);
    if (arquivo) {
        const int comprimento = limite + 14;
        int indice = 0;
        String linha;
        int tamanho, posicao;
        char caracter;
        while (arquivo.available()) {
            caracter = arquivo.read();
            if (caracter != 13) {
                if (caracter != 10)
                    linha += caracter;
                else {
                    tamanho = linha.length();
                    if ((tamanho >= limite) && (tamanho <= comprimento)) {
                        alunos[indice].registro = linha.substring(0, limite);
                        posicao = linha.indexOf(separador);
                        alunos[indice].senha = (posicao != -1) ? linha.substring(5, posicao) : "";
                        alunos[indice].cartao = (posicao != -1) ? linha.substring(posicao + 1) : "";
                        ++indice;
                    }
                    linha = "";
                }
            }
        }
        arquivo.close();
        atual = indice;
        status = 0;
    }
    return status;

}


int criaBase() {

    int status = -1;
    File arquivo = SD.open(base, FILE_WRITE);
    if (arquivo) {
        arquivo.close();
        atual = 0;
        status = 0;
    }
    return status;

}


void menuPrincipal() {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("* MANUAL      0");
    lcd.setCursor(0, 1);
    lcd.print("# CARTAO     ADM");

}


void acessoTeclado() {

    int fase = 0;
    int coluna = 0;
    String registro, senha;
    int indice;
    char tecla;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ALUNO:");
    for (;;) {
        tecla = teclado.waitForKey();
        if ((coluna < limite) && ((tecla == '1') || (tecla == '2') || (tecla == '3') || (tecla == '4') || (tecla == '5') ||
                                  (tecla == '6') || (tecla == '7') || (tecla == '8') || (tecla == '9') || (tecla == '0'))) {
            lcd.setCursor(coluna, 1);
            if (fase == 0) {
                lcd.print(tecla);
                registro += tecla;
            }
            else {
                lcd.print('*');
                senha += tecla;
            }
            coluna++;
        }
        else if (tecla == '#') {
            if (fase == 0) {
                for (indice = 0; indice < atual; indice++)
                    if (alunos[indice].registro.compareTo(registro) == 0)
                        break;
                if (indice < atual) {
                    fase = 1;
                    coluna = 0;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("SENHA:");
                }
                else {
                    aviso("RA INVALIDO.");
                    break;
                }
            }
            else {
                if (alunos[indice].senha.compareTo(senha) == 0)
                    permitido();
                else
                    negado();
                break;
            }
        }
        else if (tecla == 'D')
            break;
    }

}


void acessoCartao() {

    String cartao;
    if (recuperaCartao(&cartao) == 0) {
        int indice = 0;
        for (; indice < atual; indice++)
            if (alunos[indice].cartao.compareTo(cartao) == 0)
                break;
        if (indice != atual)
            permitido();
        else
            negado();
    }
    else
        aviso("ERRO LEITURA.");

}


void menuAdministrador() {

    if (autorizado(0) == 0) {
        int processado = 0;
        char tecla;
        for (;;) {
            if (processado == 0) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("AD A  RM B  LO C");
                lcd.setCursor(0, 1);
                lcd.print("SV *  RC 0  LM #");
                processado = 1;
            }
            tecla = teclado.waitForKey();
            if ((tecla == 'A') || (tecla == 'B') || (tecla == 'C') || (tecla == 'D') || (tecla == '*') || (tecla == '#') || (tecla == '0')) {
                if (tecla == 'A')
                    criar();
                else if (tecla == 'B')
                    remover();
                else if (tecla == 'C')
                    menuLog();
                else if (tecla == '*')
                    salvar();
                else if (tecla == '0')
                    recarregar();
                else if (tecla == '#')
                    limpar();
                else
                    break;
                processado = 0;
            }
        }
    }

}


void criar() {

    if (autorizado(1) == 0)
        if (atual < maximo) {
            int fase = 0;
            int coluna = 0;
            String registro, senha, cartao;
            char tecla;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("ALUNO:");
            for (;;) {
                tecla = teclado.waitForKey();
                if ((coluna < limite) && ((tecla == '1') || (tecla == '2') || (tecla == '3') || (tecla == '4') || (tecla == '5') ||
                                          (tecla == '6') || (tecla == '7') || (tecla == '8') || (tecla == '9') || (tecla == '0'))) {
                    lcd.setCursor(coluna, 1);
                    if (fase == 0) {
                        lcd.print(tecla);
                        registro += tecla;
                    }
                    else {
                        lcd.print('*');
                        senha += tecla;
                    }
                    coluna++;
                }
                else if (tecla == '#') {
                    if ((fase == 0) && (coluna == limite)) {
                        coluna = 0;
                        fase = 1;
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.print("SENHA:");
                    }
                    else if (fase == 1) {
                        if (modoAdicionar(&cartao) == 0) {
                            int indice = 0;
                            for (; indice < atual; indice++)
                                if (alunos[indice].registro.compareTo(registro) == 0)
                                    break;
                            if (indice == atual) {
                                alunos[atual].registro = registro;
                                alunos[atual].senha = senha;
                                alunos[atual].cartao = cartao;
                                atual++;
                            }
                            else {
                                alunos[atual].senha = senha;
                                alunos[atual].cartao = cartao;
                            }
                            mensagem("ALUNO CRIADO.");
                            break;
                        }
                        else
                            break;
                    }
                }
                else if (tecla == 'D')
                    break;
            }
        }
        else
            aviso("LIMITE MAXIMO.");
    else
        aviso("SEM AUTORIZACAO.");

}


void remover() {

    if (autorizado(1) == 0) {
        int coluna = 0;
        String registro;
        char tecla;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ALUNO:");
        for (;;) {
            tecla = teclado.waitForKey();
            if ((coluna < limite) && ((tecla == '1') || (tecla == '2') || (tecla == '3') || (tecla == '4') || (tecla == '5') ||
                                      (tecla == '6') || (tecla == '7') || (tecla == '8') || (tecla == '9') || (tecla == '0'))) {
                lcd.setCursor(coluna, 1);
                lcd.print(tecla);
                registro += tecla;
                coluna++;
            }
            else if (tecla == '#') {
                int indice = 0;
                for (; indice < atual; indice++)
                    if (alunos[indice].registro.compareTo(registro) == 0)
                        break;
                if (indice != atual) {
                    int ultimo = atual - 1;
                    if (indice != ultimo)
                        alunos[indice] = alunos[ultimo];
                    atual--;
                    mensagem("ALUNO EXCLUIDO.");
                }
                else
                    aviso("INEXISTENTE.");
                break;
            }
            else if (tecla == 'D')
                break;
        }
    }
    else
        aviso("SEM AUTORIZACAO.");

}


void salvar() {

    if (autorizado(1) == 0) {
        File arquivo = SD.open(base, O_TRUNC|O_WRITE);
        if (arquivo) {
            for (int indice = 0; indice < atual; indice++) {
                arquivo.print(alunos[indice].registro);
                arquivo.print(alunos[indice].senha);
                arquivo.print(separador);
                arquivo.print(alunos[indice].cartao);
                arquivo.print('\n');
            }
            arquivo.close();
            mensagem("BASE ATUALIZADA.");
        }
        else
            aviso("FALHA AO GRAVAR.");
    }
    else
        aviso("SEM AUTORIZACAO.");

}


void recarregar() {

    if (autorizado(1) == 0)
        if (carregaBase() == 0)
            mensagem("RECARREGADO.");
        else
            aviso("FALHA NA CARGA.");
    else
        aviso("SEM AUTORIZACAO.");

}


void limpar() {

    if (autorizado(1) == 0)
        if (((SD.exists(base) == 0) || (SD.remove(base) == 1)) && (criaBase() == 0))
            mensagem("BASE LIMPA.");
        else
            aviso("FALHA AO LIMPAR.");
    else
        aviso("SEM AUTORIZACAO.");

}


void menuLog() {

    if (autorizado(1) == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("0 - BASE");
        lcd.setCursor(0, 1);
        lcd.print("1 - CARTAO");
        char tecla;
        for(;;) {
            tecla = teclado.waitForKey();
            if (tecla == '0') {
                despejaBase();
                break;
            }
            else if (tecla == '1') {
                despejaCartao();
                break;
            }
            else if (tecla == 'D')
                break;
        }
    }
    else
        aviso("SEM AUTORIZACAO.");

}


int modoAdicionar(String* const cartao) {

    int status = -1;
    if (cartao) {
        int processado = 0;
        char tecla;
        for(;;) {
            if (processado == 0) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("0 - SEM CARTAO.");
                lcd.setCursor(0, 1);
                lcd.print("1 - COM CARTAO.");
                processado = 1;
            }
            tecla = teclado.waitForKey();
            if (tecla == '0') {
                *cartao = "";
                status = 0;
                break;
            }
            else if (tecla == '1') {
                if (recuperaCartao(cartao) == 0) {
                    status = 0;
                    break;
                }
                else
                    aviso("ERRO LEITURA.");
                processado = 0;
            }
            if (tecla == 'D')
                break;
        }
    }
    return status;

}


int recuperaCartao(String* const cartao) {

    int status = -1;
    if (cartao) {
        digitalWrite(ambar, HIGH);
        SPI.begin();
        rfid.PCD_Init();
        MFRC522::PICC_Type piccType;
        char caracter;
        byte indice, valor;
        for (int falhas = 0; falhas < 5; falhas++)
            if ((rfid.PICC_IsNewCardPresent()) && (rfid.PICC_ReadCardSerial())) {
                piccType = rfid.PICC_GetType(rfid.uid.sak);
                *cartao = "";
                for (indice = 0; indice < rfid.uid.size; indice++) {
                    valor = (rfid.uid.uidByte[indice] >> 4) & 0x0F;
                    caracter = (valor  < 0xA) ? (48 + valor) : (65 + valor - 0xA);
                    cartao->concat(caracter);
                    valor = (rfid.uid.uidByte[indice] >> 0) & 0x0F;
                    caracter = (valor  < 0xA) ? (48 + valor) : (65 + valor - 0xA);
                    cartao->concat(caracter);
                }
                status = 0;
                break;
            }
            else
                delay(500);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        SPI.end();
        digitalWrite(ambar, LOW);
    }
    return status;

}


void despejaBase() {

    Serial.begin(9600);
    Serial.print("\n:: ");
    Serial.print(atual);
    Serial.print(" ::\n");
    for (int indice = 0; indice < atual; indice++) {
        Serial.print("A[" + alunos[indice].registro);
        Serial.print("] S[" + alunos[indice].senha);
        Serial.print("] C[" + alunos[indice].cartao);
        Serial.print("]\n");
    }
    Serial.end();

}


void despejaCartao() {

    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();
    MFRC522::PICC_Type piccType;
    byte indice;
    for (int falhas = 0; falhas < 5; falhas++)
        if ((rfid.PICC_IsNewCardPresent()) && (rfid.PICC_ReadCardSerial())) {
            piccType = rfid.PICC_GetType(rfid.uid.sak);
            Serial.print("ID do cartao: ");
            for (indice = 0; indice < rfid.uid.size; indice++) {
                Serial.print(rfid.uid.uidByte[indice] < 0x10 ? " 0" : " ");
                Serial.print(rfid.uid.uidByte[indice], HEX);
            }
            Serial.print("\nLeitura concluida com sucesso.\n");
            break;
        }
        else {
            Serial.print("Falha na leitura, tentando novamente...\n");
            delay(500);
        }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    SPI.end();
    Serial.end();

}


int autorizado(const int nivel) {

    int status = -1;
    if ((nivel == 0) || (nivel == 1)) {
        const String* alvo = (nivel == 0) ? &administrador : &operador;
        int coluna = 0;
        String senha;
        char tecla;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("SENHA:");
        for (;;) {
            tecla = teclado.waitForKey();
            if ((coluna < limite) && ((tecla == '1') || (tecla == '2') || (tecla == '3') || (tecla == '4') || (tecla == '5') ||
                                      (tecla == '6') || (tecla == '7') || (tecla == '8') || (tecla == '9') || (tecla == '0'))) {
                lcd.setCursor(coluna, 1);
                lcd.print('*');
                senha += tecla;
                coluna++;
            }
            else if (tecla == '#') {
                if (alvo->compareTo(senha) == 0)
                    status = 0;
                break;
            }
            else if (tecla == 'D')
                break;
        }
    }
    return status;

}


void permitido() {

    digitalWrite(verde, HIGH);
    mensagem("AUTORIZADO.");
    digitalWrite(verde, LOW);

}


void negado() {

    digitalWrite(vermelho, HIGH);
    aviso("ACESSO NEGADO.");
    digitalWrite(vermelho, LOW);

}


void mensagem(String texto) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(".::    ..    ::.");
    lcd.setCursor(0, 1);
    lcd.print(texto);
    delay(2000);

}


void aviso(String texto) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(".::   ERRO   ::.");
    lcd.setCursor(0, 1);
    lcd.print(texto);
    delay(2000);

}
