# RTSP In-Out Stream with GStreamer (Multithreaded Encoding and Decoding)

This RTSP In-Out Stream implementation processes an RTSP input, performing multithreaded encoding and decoding with GStreamer for improved performance.

## Code Overview

- **GStreamer Setup**: Initializes GStreamer and sets up an RTSP server with a media factory for handling incoming RTSP streams.
- **Dynamic Pad Linking**: Implements a `pad-added` signal handler to connect the dynamic source pad to the pipeline, allowing for flexible RTSP input handling.
- **Pipeline Components**:
  - Receives an RTSP input stream (`rtspsrc`) and handles H.264 depayloading (`rtph264depay`).
  - Decodes the video stream with `nvv4l2decoder` and converts the format with `nvvidconv`.
  - Uses `nvv4l2h264enc` for multithreaded encoding to optimize performance during high-demand processing.
  - Encodes and repackages the stream with H.264 RTP payloading (`rtph264pay`) for RTSP output.
- **Pipeline Characteristics**:
  - Encoding and decoding are multithreaded, reducing latency and enabling smoother handling of high-bitrate streams.
  - Pipeline includes a `queue` element for buffering, which further supports multithreaded processing.

## Important Notes

- **VLC Compatibility**: VLC may encounter connection issues due to the increased latency caused by high processing demands.
- **VLC alternative** : using the following bash command to start works better than vlc
```bash
gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/output ! decodebin ! videoconvert ! autovideosink
```
