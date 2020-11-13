# docker pull aler9/rtsp-simple-server

xterm -e "docker run --rm -it --network=host aler9/rtsp-simple-server" &

sleep 1 && xterm -e "ffmpeg -f v4l2 -i /dev/video0 -f rtsp rtsp://localhost:8554/mystream" &

#sleep 2 && vlc rtsp://localhost:8554/mystream
