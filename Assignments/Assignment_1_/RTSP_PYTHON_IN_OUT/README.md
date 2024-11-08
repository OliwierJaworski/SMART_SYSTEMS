# RTSP Input/Output Streaming with GStreamer in PYTHON

This application sets up an RTSP input and output pipeline using GStreamer in PYTHON. It receives an RTSP stream, processes it, and re-encodes the output into a new RTSP stream via an RTSP server.

## Code Overview

- **GStreamer Setup**: Initializes GStreamer and sets up an RTSP server to handle the input and output streams.
- **Stream Handling**: The script creates a single pipeline to receive and process a stream from a specified RTSP source and output it via the RTSP server.

### Pipeline Components

The pipeline consists of the following elements:
- **RTSP Input (`rtspsrc`)**: Receives the input stream from a given RTSP URL (`rtsp://192.168.0.145:8554/xx`).
- **Depayloading (`rtph264depay`)**: Depayloads RTP H.264 packets, extracting the video stream for decoding.
- **Queue (`queue`)**: Buffers data to manage the flow of packets between elements, improving pipeline stability.
- **Decoding (`nvv4l2decoder`)**: Decodes the H.264 video stream using hardware acceleration.
- **Format Conversion (`nvvidconv`)**: Converts the decoded video into the desired format for re-encoding.
- **Encoding (`nvv4l2h264enc`)**: Re-encodes the video back to H.264 with configurable bitrate and I-frame interval.
- **RTP Payloading (`rtph264pay`)**: Encodes the video stream into RTP packets for transmission over RTSP.

### Error and EOS Handling

- **Dynamic Pad Linking**: The `pad-added` callback dynamically links the source pad from the RTSP input to the depayloading element.
- **Error Handling**: The bus watches for errors and end-of-stream (EOS) messages. If an error occurs or the stream ends, the main loop quits gracefully.

### RTSP Server Setup

- **RTSP Server**: An RTSP server is created and configured with a media factory that uses a GStreamer launch string to define the processing pipeline.
- **Output Mount Point**: The output stream is available at `rtsp://127.0.0.1:8554/output`, which can be accessed by clients for viewing.

### RTSP Output

The processed RTSP stream is made available through the following RTSP server mount point:
- `rtsp://127.0.0.1:8554/output`

## Important Notes

- **Hardware Acceleration**: The script uses `nvv4l2decoder` for hardware-accelerated video decoding and `nvv4l2h264enc` for encoding, ensuring optimal performance.
- **Error Handling**: The pipeline will stop and log errors if issues occur during streaming or processing, ensuring the application doesn't crash unexpectedly.
- **Customizable Parameters**: The properties of the pipeline elements (e.g., bitrate, latency) can be adjusted to fit different use cases.

