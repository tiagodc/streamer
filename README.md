## C++ real time video streamer
*Tiago de Conto, 13/11/2020*

A lógica do streamer está toda no arquivo `main.cpp`. Os comandos do compilador (`g++`) estão *hard coded* em `buid.sh`. O ideal seria usar o `cmake` e fazer um `makefile` apropriado... *but life is too short*.   

#### Instruções

Compilar a imagem docker e executar:
```
cd streamer
docker build -f Dockerfile -t cpp-streamer .
```

Gere o stream rtsp >> websocket:
```
cd streamer
# linux
docker run -d --rm --network=host cpp-streamer '<caminho_rtsp>' <porta_de_saida>
#windows
docker run -d --rm -p <porta_de_saida>:<porta_de_saida> cpp-streamer '<caminho_rtsp>' <porta_de_saida>
```
**`'<caminho_rtsp>'` precisa estar entre aspas simples caso a string tenha alguma exclamação `!`*

Confira o streaming no browser. A porta de teste *default* é `9090`, caso utilize outra é necessário alterar o arquivo `index.html` a corrigir a porta do websocket.
```
cd streamer
firefox index.html
```


#### Bônus: teste com webcam

Para facilitar os testes em ambiente de desenvolvimento é conveniente gerar o streaming RTSP da webcam local. Para isso são necessários o ffmpeg e o `rtsp-simple-server` (dockerizado):

```
sudo apt-get install ffmpeg
docker pull aler9/rtsp-simple-server
```

Num terminal, abra o servidor rtsp:
```
docker run --rm -it --network=host aler9/rtsp-simple-server
```

Em outro terminal redirecione a webcam para o servidor RTSP:
```
ffmpeg -f v4l2 -i /dev/video0 -f rtsp rtsp://localhost:8554/mystream
```

Para verificar o streaming RTSP no VLC player:
```
vlc rtsp://localhost:8554/mystream
```

Abra o conversor RTSP >> websocket para o streaming rtsp:
```
docker run --rm -p 9090:9090 -d cpp-streamer 'rtsp://localhost:8554/mystream' 9090
```

Confira o streaming no browser:
```
firefox index.html
```
**caso utilize uma porta diferente da `9090`, não esqueça de alterar também o arquivo index.html.*