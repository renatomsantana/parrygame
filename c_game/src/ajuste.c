/*
 * ajuste.c - os valores de ajuste.h viram as Settings de um duelo.
 */
#include "core.h"

void settings_default(Settings *s) {
    s->renPosture = AJ_VIDA_INICIAL;
    s->badBossRecover = AJ_ERRO_MESTRE_RECUPERA;
    s->goodBossDamage = AJ_BOM_POSTURA;
    s->goodRenCost = AJ_BOM_CUSTO;
    s->perfectBossDamage = AJ_PERFEITO_POSTURA;
    s->perfectHeal = AJ_PERFEITO_CURA;
    s->inputCooldown = AJ_ENTRE_GESTOS;
    s->lateGrace = AJ_TOLERANCIA_TARDIA;
    s->latency = 0;
    s->audioLead = 0;
    s->attackLead = AJ_LAMINA_PARTE;
    s->recovery = AJ_PAUSA_SEQUENCIA;
    s->sealRecovery = AJ_PAUSA_SELO;
    s->sealHeal = AJ_SELO_CURA;
    s->firstWindupDelay = AJ_PAUSA_INICIO;
    s->pressureSpeed = AJ_PRESSA;
    s->comboGap = AJ_PAUSA_NA_CADEIA;
    s->minChainGap = AJ_CADEIA_MIN;
    s->goodHitstop = AJ_HITSTOP_BOM;
    s->perfectHitstop = AJ_HITSTOP_PERFEITO;
    s->badHitstop = AJ_HITSTOP_ERRO;
    s->breakHitstop = AJ_HITSTOP_QUEBRA;
    s->postureGrowth = AJ_VIDA_POR_MESTRE;
    s->perfectGrowth = AJ_PERFEITO_POSTURA_NIVEL;
    s->goodGrowth = AJ_BOM_POSTURA_NIVEL;
}

void settings_for_level(Settings *s, int defeated) {
    if (defeated < 0) defeated = 0;
    s->renPosture += s->postureGrowth * defeated;
    s->perfectBossDamage += s->perfectGrowth * defeated;
    s->goodBossDamage += s->goodGrowth * defeated;
}
