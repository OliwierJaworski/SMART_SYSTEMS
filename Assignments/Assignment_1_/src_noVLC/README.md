# RTSP In-Out Stream with GStreamer without multithreading

This implementation of the RTSP In-Out Stream does basic decoding, video conversion, encoding, and repackaging, without multithreading.

## Code Overview

- **GStreamer Setup**: Initializes GStreamer and creates an RTSP server with a media factory for handling incoming RTSP input.
- **Dynamic Pad Linking**: A `pad-added` signal handler links the dynamic source pad to the pipeline for seamless RTSP input handling.
- **Pipeline**:
  - Receives an RTSP input stream (`rtspsrc`).
  - Performs H.264 depayloading (`rtph264depay`), decoding (`omxh264dec`), and format conversion (`videoconvert`).
  - Encodes (`x264enc`) the processed video for RTSP output with H.264 RTP payloading (`rtph264pay`).
- **Pipeline Limitations**:
  - No multithreading for decoding/encoding, leading to potential stuttering or desync if processing demands exceed available headroom.
  - Default settings may limit smoothness if the stream resolution or bitrate is high.

## Important Notes

- **Note on VLC Compatibility**: VLC may be unable to connect due to latency issues with the RTSP server.
- **VLC alternative** : using the following bash command to start works better than vlc
```bash
gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/output ! decodebin ! videoconvert ! autovideosink
```