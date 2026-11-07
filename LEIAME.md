# Renan Santos Crusher

Plugin VST3 de efeito (distorção tanh + bitcrusher + redução de taxa) com visual escuro/laranja.
Feito em JUCE. Não usa voz nem áudio de ninguém.

## Compilar sem instalar nada no PC (GitHub Actions)

1. Crie um repositório novo no GitHub (pode ser privado).
2. Envie todo o conteúdo desta pasta para ele, **incluindo a pasta `.github`**
   (no site: "Add file" > "Upload files" e arraste tudo).
3. Vá na aba **Actions** do repositório. A compilação começa sozinha
   (ou clique em "Compilar plugin" > "Run workflow").
4. Quando ficar verde (leva uns 10 a 15 minutos na primeira vez), abra a execução
   e baixe o arquivo **RenanSantosCrusher-VST3** na seção *Artifacts*.
5. Descompacte e copie a pasta `Renan Santos Crusher.vst3` para
   `C:\Program Files\Common Files\VST3\`.
6. No FL Studio: Options > Manage plugins > Find installed plugins.
   Ele aparece como "Renan Santos Crusher".

O JUCE é baixado automaticamente pelo CMake durante a compilação.

## Parâmetros

- Pressão: quanto de distorção entra
- Bits: resolução do bitcrusher
- Taxa: reduz a taxa de amostragem (segura 1 a cada N amostras)
- Mix: seco/molhado
- Saída: volume final em dB
