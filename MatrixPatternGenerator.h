// Este arquivo foi disponibilizado como material do curso Hackeando Jogos
// Desenvolvedor: Crackiller
// https://www.hackeandojogos.com
// https://www.cheatsplace.com
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include "pch.h"

#ifndef _MATRIX_PATTERN_GENERATOR_
#define _MATRIX_PATTERN_GENERATOR_

// Nível de precisão a ser gerada no pattern
enum PATTERN_FLOAT_BYTE_PRECISION
{
    PRECISION_HIGHER_LAST_NIBBLE,              // MENOR PRECISÃO    | 4 bits de cima do último byte de um float da matrix = sizeof(float) = 4 bytes. Exemplo: 1.0f = 00 00 80 3F | Onde 3 é o nibble de cima e F é o nibble de baixo
    PRECISION_FULL_LAST_BYTE,                  // MAIOR PRECISÃO    | Último byte completo do float. Exemplo: 1.0f = 00 00 80 3F | Onde 3F é o último byte completo
};

// Convenção do sistema de coordenadas / lateralidade
// LEFT_HANDED / CANHOTO = Forward é positivo para frente e negativo para trás com a câmera na origem (X = 0, Y = 0, Z = 0) e as rotações Pitch = 0, Yaw = 0;
// RIGHT_HANDED / DESTRO = Forward é positivo para trás e negativo para frente com a câmera na origem (X = 0, Y = 0, Z = 0) e as rotações Pitch = 0, Yaw = 0;
// UNKNOWN_HANDEDNESS / DESCONHECIDO = Convenção desconhecida. Vai gerar um caractere coringa ? no nibble de cima do campo que contém a convenção na matriz;
enum COORDINATE_SYSTEM_HANDEDNESS
{
    LEFT_HANDED,
    RIGHT_HANDED,
    UNKNOWN_HANDEDNESS
};

// Pares do último byte da escala em x e y na ProjectionMatrix, esses valores são as possibilidades mais comuns encontradas nesse tipo de matriz.
// Os pares 0 e 1 são comuns em jogos quando não há zoom/scope aplicado.
// Os pares 2 e 3 são comuns em jogos quando o jogador está usando zoom/scope.
enum PROJECTION_SCALE_LASTBYTE_PAIR
{                                              // Layout geral de uma ProjectionMatrix de perspectiva
    PM_BYTE_PAIR_0 = 0x3F3F,                   // ?? ?? ?? XX | 00 00 00 00 | 00 00 00 00 | 00 00 00 00 // -> ?? ?? ?? XX = xScale
    PM_BYTE_PAIR_1 = 0x403F,                   // 00 00 00 00 | ?? ?? ?? XX | 00 00 00 00 | 00 00 00 00 // -> ?? ?? ?? XX = yScale
    PM_BYTE_PAIR_2 = 0x3F40,                   // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 80 ?F // -> ?F: (3F = +1.0f = LEFT_HANDED), (BF = -1.0f = RIGHT_HANDED)
    PM_BYTE_PAIR_3 = 0x4040,                   // 00 00 00 00 | 00 00 00 00 | ?? ?? ?? ?? | 00 00 00 00
};

// Convenção dos eixos usada pelo jogo
// Muitos jogos não usam a convenção padrão XYZ, então é necessário cobrir todas as possibilidades.
enum AXIS_CONVENTION
{
    XYZ,
    XZY,
    YXZ,
    YZX,
    ZXY,
    ZYX,
};

// NÃO USADO POR ENQUANTO
// Convenção das matrizes
// Especifica a disposição dos eixos na matrix, ou seja, em linhas = ROW_MAJOR ou colunas = COLUMN_MAJOR
enum MATRIX_MAJORNESS
{
    MATRIX_ROW_MAJOR,
    MATRIX_COLUMN_MAJOR,
};

#define MATRIX_PATTERN_STRING_SIZE 192 // sizeof(D3DXMATRIX) = 64 bytes -> são 2 caracteres por byte, logo -> 64 * 2 = 128 -> + 63 espaços ' ' entre cada byte = 191 -> + 1 byte 0x00 para o fim da string = 192

char ViewMatrixPattern[MATRIX_PATTERN_STRING_SIZE];                 // String com o pattern gerado pela função GenerateViewMatrixPattern
char ProjectionMatrixPattern[MATRIX_PATTERN_STRING_SIZE];           // String com o pattern gerado pela função GenerateProjectionMatrixPattern

#define MyD3DX_PI    ((FLOAT)  3.141592654f)                        // Constante PI

#define MyD3DXToRadian( degree ) ((degree) * (MyD3DX_PI / 180.0f))  // Macro para converter graus para radianos
#define MyD3DXToDegree( radian ) ((radian) * (180.0f / MyD3DX_PI))  // Macro para converter radianos para graus

#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)                            // Obtém o nibble de cima de um byte, exemplo: 3F -> 3 é nibble de cima
#define LO_NIBBLE(b) ((b) & 0x0F)                                   // Obtém o nibble de baixo de um byte, exemplo: 3F -> F é nibble de baixo

// Obtém o último byte de uma váriável de 4 bytes usando seu endereço, no nosso caso passaremos ponteiros de valores do tipo float (sizeof(float) = 4 bytes).
BYTE GetLastByte(PVOID data)
{
    PBYTE pByte = (PBYTE)data;

    return pByte[3];
}

// Preenche o pattern da ViewMatrix
void FillPatternMatrixRows(FLOAT X, FLOAT Y, FLOAT Z, PATTERN_FLOAT_BYTE_PRECISION precision, bool lastCase)
{
    // Atribui os componententes X, Y e Z do eixo direcional Forward/Frente a um vetor para facilitar a criação de um loop.
    FLOAT fVector[] = {X, Y, Z};

    // Percorre por cada elemento do vetor
    for (size_t i = 0; i < 3; i++)
    {
        BYTE bLastByte = GetLastByte(&fVector[i]);

        // Variável de controle para saber se o valor gerado pelo nosso cálculo em GenerateViewMatrixPattern está dentro dos limites de -1 até +1
        bool isWithinRange = false;

        BYTE HighNibble = HI_NIBBLE(bLastByte);
        BYTE LowNibble = LO_NIBBLE(bLastByte);

        // Essa parte é necessária porque às vezes o cálculo pode gerar valores que podem quebrar o pattern, como por exemplo: 00 00 00 80
        // Em tese, esse valor se fosse representado como float seria = 0.
        // Porém o que acontece é que o hexadecimal da ViewMatrix do jogo pode ter lá: 00 00 00 00 ou invés de 00 00 00 80
        // E se compararmos esse valor com o pattern anterior não iremos encontrar o que estamos procurando
        // Então a melhor opção é verificar se algum número não esperado foi gerado, e desconsiderar esse pattern, substituindo por um genérico.
        switch (HighNibble)
        {
        case 0x03:
        case 0x0B:
            // Qualquer número válido na ViewMatrix, terá o último byte sendo no mínimo 38 e no máximo 3F (positivos) OU no mínimo B8 e no máximo BF (negativos)
            // Isso considerando apenas o último byte como referência e os outros 3 sendo iguais a 00
            // Aqui verificamos se o valor gerado pelo cálculo está dentro desse requisito
            if (LowNibble > 0x07)
            {
                isWithinRange = true;
            }
            break;
        default:
            break;
        }

        // Se sair do limite de -1 até +1
        if(!isWithinRange)
        {
            // Gera um padrão genérico, ou seja, mais abrangente/menos específico.
            // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 00 00
            sprintf(&ViewMatrixPattern[strlen(ViewMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00 ");
        }
        else
        {
            // Se estiver dentro dos limites, prosseguimos para o nivel de precisão passado como parâmetro.
            switch (precision)
            {
                // Gera um padrão baseado apenas no nibble de cima do último byte
                case PRECISION_HIGHER_LAST_NIBBLE:
                // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? X? | 00 00 00 00
                sprintf(&ViewMatrixPattern[strlen(ViewMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? %01X? 00 00 00 00 ", HighNibble);
                break;
                // Gera um padrão baseado no último byte de cada float (especificamente nos floats que representam o eixo Z / Forward) na matriz
                case PRECISION_FULL_LAST_BYTE:
                    // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? XX | 00 00 00 00
                sprintf(&ViewMatrixPattern[strlen(ViewMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? %02X 00 00 00 00 ", bLastByte);
                break;
            }
        }
    }

    if (!lastCase)
    {
        // Última linha padrão de uma ViewMatrix = X, Y, Z, 1.0f
        // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 00 3F
        sprintf(&ViewMatrixPattern[strlen(ViewMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F");
    }
    else
    {
        // Em caso de não encontrar nenhum resultado satisfatório com os possíveis patterns, tente setar lastCase para true.
        // Pode ser que você encontre a última linha de uma ViewMatrix = X, Y, Z, 0.0f
        sprintf(&ViewMatrixPattern[strlen(ViewMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 00 00");
    }
}

void GenerateViewMatrixPattern(FLOAT yaw, FLOAT pitch, bool flipSpitchSign, AXIS_CONVENTION convention, PATTERN_FLOAT_BYTE_PRECISION precision, bool lastCase)
{
    ZeroMemory(ViewMatrixPattern, sizeof(ViewMatrixPattern));

    FLOAT spitch, cpitch, syaw, cyaw;

    spitch = sinf(MyD3DXToRadian(pitch));
    cpitch = cosf(MyD3DXToRadian(pitch));
    syaw = sinf(MyD3DXToRadian(yaw));
    cyaw = cosf(MyD3DXToRadian(yaw));

    if (flipSpitchSign)
    {
        spitch = -spitch;
    }

    FLOAT X, Y, Z;

    switch (convention)
    {
    case XYZ:
        X = cpitch * syaw;
        Y = spitch;
        Z = cpitch * cyaw;
        break;
    case XZY:
        X = cpitch * syaw;
        Y = cpitch * cyaw;
        Z = spitch;
        break;
    case YXZ:
        X = spitch;
        Y = cpitch * syaw;
        Z = cpitch * cyaw;
        break;
    case YZX:
        X = spitch;
        Y = cpitch * cyaw;
        Z = cpitch * syaw;
        break;
    case ZXY:
        X = cpitch * cyaw;
        Y = cpitch * syaw;
        Z = spitch;
        break;
    case ZYX:
        X = cpitch * cyaw;
        Y = spitch;
        Z = cpitch * syaw;
        break;
    default:
        break;
    }

    FillPatternMatrixRows(X, Y, Z, precision, lastCase);
}

void GenerateProjectionMatrixPattern(PROJECTION_SCALE_LASTBYTE_PAIR scaleBytePair, COORDINATE_SYSTEM_HANDEDNESS handedness, PATTERN_FLOAT_BYTE_PRECISION precision)
{
    ZeroMemory(ProjectionMatrixPattern, sizeof(ProjectionMatrixPattern));

    BYTE xScaleLastByte = ((PBYTE)&scaleBytePair)[0];
    BYTE yScaleLastByte = ((PBYTE)&scaleBytePair)[1];

    BYTE xHighNibble = HI_NIBBLE(xScaleLastByte);
    BYTE yHighNibble = HI_NIBBLE(yScaleLastByte);

    switch (precision)
    {
    case PRECISION_HIGHER_LAST_NIBBLE:
        // ?? ?? ?? X? | 00 00 00 00 | 00 00 00 00 | 00 00 00 00
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "?? ?? ?? %01X? 00 00 00 00 00 00 00 00 00 00 00 00 ", xHighNibble);
        // 00 00 00 00 | ?? ?? ?? X? | 00 00 00 00 | 00 00 00 00
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "00 00 00 00 ?? ?? ?? %01X? 00 00 00 00 00 00 00 00 ", yHighNibble);
        break;
    case PRECISION_FULL_LAST_BYTE:
        // ?? ?? ?? XX | 00 00 00 00 | 00 00 00 00 | 00 00 00 00
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "?? ?? ?? %02X 00 00 00 00 00 00 00 00 00 00 00 00 ", xScaleLastByte);
        // 00 00 00 00 | ?? ?? ?? XX | 00 00 00 00 | 00 00 00 00
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "00 00 00 00 ?? ?? ?? %02X 00 00 00 00 00 00 00 00 ", yScaleLastByte);
        break;
    default:
        break;
    }

    switch (handedness)
    {
    case LEFT_HANDED:
        // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 80 3F
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 3F ");
        break;
    case RIGHT_HANDED:
        // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 80 BF
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 BF ");
        break;
    case UNKNOWN_HANDEDNESS:
        // ?? ?? ?? ?? | ?? ?? ?? ?? | ?? ?? ?? ?? | 00 00 80 ?F
        sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 80 ?F ");
        break;
    default:
        break;
    }
    
    // 00 00 00 00 | 00 00 00 00 | ?? ?? ?? ?? | 00 00 00 00
    sprintf(&ProjectionMatrixPattern[strlen(ProjectionMatrixPattern)], "00 00 00 00 00 00 00 00 ?? ?? ?? ?? 00 00 00 00");
}

bool ShowResultMessageBox = true;

// Função geral para criar os patterns das 2 matrizes
void GenerateMatrixPatterns(FLOAT Yaw, FLOAT Pitch)
{
    // Menor precisão, irá encontrar muitos endereços, porém nem todos são endereços válidos.
    // É útil para fazer uma pesquisa mais abrangente em caso da primeira não funcionar.
    //GenerateViewMatrixPattern(Yaw, Pitch, true, ZXY, PRECISION_HIGHER_LAST_NIBBLE, false);
    
    // Maior precisão, irá encontrar menos endereços, porém provavelmente são os endereços corretos.
    GenerateViewMatrixPattern(Yaw, Pitch, true, ZXY, PRECISION_FULL_LAST_BYTE, false);

    // Gera um pattern da ProjectionMatrix
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, LEFT_HANDED, PRECISION_FULL_LAST_BYTE);


    if (ShowResultMessageBox)
    {
        // Para copiar o pattern no Cheat Engine, use: MenuD3D.ViewMatrixPattern
        MessageBoxA(0, ViewMatrixPattern, "ViewMatrix Pattern", 0);
        // Para copiar o pattern no Cheat Engine, use: MenuD3D.ProjectionMatrixPattern
        MessageBoxA(0, ProjectionMatrixPattern, "ProjectionMatrix Pattern", 0);
        ShowResultMessageBox = false;
    }
}

/*

    // Possibilidades de chamada para criação de pattern da ViewMatrix

    GenerateViewMatrixPattern(Yaw, Pitch, true, XYZ, PRECISION_FULL_LAST_BYTE, false);
    GenerateViewMatrixPattern(Yaw, Pitch, true, XZY, PRECISION_FULL_LAST_BYTE, false);
    GenerateViewMatrixPattern(Yaw, Pitch, true, YXZ, PRECISION_FULL_LAST_BYTE, false);
    GenerateViewMatrixPattern(Yaw, Pitch, true, YZX, PRECISION_FULL_LAST_BYTE, false);
    GenerateViewMatrixPattern(Yaw, Pitch, true, ZXY, PRECISION_FULL_LAST_BYTE, false);
    GenerateViewMatrixPattern(Yaw, Pitch, true, ZYX, PRECISION_FULL_LAST_BYTE, false);


    // Possibilidades de chamada para criação de pattern da ProjectionMatrix de perspectiva

    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, LEFT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, LEFT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, RIGHT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, RIGHT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, UNKNOWN_HANDEDNESS, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_0, UNKNOWN_HANDEDNESS, PRECISION_FULL_LAST_BYTE);

    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, LEFT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, LEFT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, RIGHT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, RIGHT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, UNKNOWN_HANDEDNESS, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_1, UNKNOWN_HANDEDNESS, PRECISION_FULL_LAST_BYTE);

    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, LEFT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, LEFT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, RIGHT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, RIGHT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, UNKNOWN_HANDEDNESS, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_2, UNKNOWN_HANDEDNESS, PRECISION_FULL_LAST_BYTE);

    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, LEFT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, LEFT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, RIGHT_HANDED, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, RIGHT_HANDED, PRECISION_FULL_LAST_BYTE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, UNKNOWN_HANDEDNESS, PRECISION_HIGHER_LAST_NIBBLE);
    GenerateProjectionMatrixPattern(PM_BYTE_PAIR_3, UNKNOWN_HANDEDNESS, PRECISION_FULL_LAST_BYTE);


*/

#endif