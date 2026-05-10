# Projeto Catapulta ESP32 - Introdução a Projetos de Engenharia

Sistema de controle para catapulta automatizada utilizando ESP32, motor de passo e servo motor.

## Funcionalidades
- **Interface Web:** Slider intuitivo para definir distância (0.5m a 4.0m).
- **Segurança:** O disparo via gatilho somente é habilitado após o motor de passo atingir a posição alvo.
- **Sinalização Visual:**
  - **LED Verde:** Sistema online e Wi-Fi ativo.
  - **LED Amarelo:** Motor em movimento (Aguarde para disparar).

## Hardware
- Microcontrolador: ESP32 DevKit V1
- Motor de Passo: 28BYJ-48 com Driver ULN2003
- Servo: SG90 (Gatilho)
- Alimentação: 4 pilhas AA (6V) com GND comum ao ESP32.

## Como usar
1. Ligue a chave geral na caixa plástica.
2. Conecte o celular à rede Wi-Fi `Catapulta_Equipe_01`.
3. Acesse `192.168.4.1` no navegador.
4. Ajuste a distância e clique em **LANÇAR!**
