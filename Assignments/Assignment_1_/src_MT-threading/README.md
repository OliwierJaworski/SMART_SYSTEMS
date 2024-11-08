# Multistream RTSP Server with GStreamer

This application processes multiple RTSP streams simultaneously. It dynamically sets up separate pipelines for each input URI and outputs them via an RTSP server with GStreamer.

## Code Overview

- **GStreamer Setup**: Initializes GStreamer and sets up an RTSP server capable of handling multiple streams.
- **Multistream Handling**: The `start_rtsp_pipeline()` function creates and starts a separate pipeline for each input stream, allowing the server to process multiple streams concurrently.
  
### Pipeline Components

Each pipeline consists of the following elements:
- **RTSP Input (`rtspsrc`)**: Receives the input stream from a given URI.
- **Depayloading (`rtph264depay`)**: Depayloads RTP H.264 packets for decoding.
- **Decoding (`nvv4l2decoder`)**: Decodes the H.264 stream using hardware acceleration.
- **Format Conversion (`nvvidconv`)**: Converts the video format as needed.
- **Multithreaded Encoding (`nvv4l2h264enc`)**: Encodes the video back to H.264 with multithreaded processing for better performance.
- **RTP Payloading (`rtph264pay`)**: Encodes the stream into RTP packets for RTSP output.

### Error and EOS Handling

- **Dynamic Pad Linking**: A `pad-added` callback dynamically links the source pad of the RTSP input stream to the pipeline's decoder.
- **Error Handling**: The bus watches for errors and end-of-stream (EOS) messages, ensuring that the server can handle failures and stream completion properly.

### RTSP Server Setup

- **RTSP Server**: A server is created and configured for each stream, with a different mount point for each input/output pair.
- **Multiple Pipelines**: The `start_rtsp_pipeline()` function is called twice in `main()` to start two pipelines for different input streams (`stream1` and `stream2`), each with its own output mount point.

### RTSP Output

Each stream is available via a unique mount point on the RTSP server:
- `rtsp://127.0.0.1:8554/output1`
- `rtsp://127.0.0.1:8554/output2`

## Important Notes

- **Multithreaded Encoding**: The use of hardware-accelerated encoding (`nvv4l2h264enc`) and decoding (`nvv4l2decoder`) ensures optimal performance, especially for high-bitrate streams.
- **Error Handling**: If an error occurs in any pipeline, it is logged and the main loop is terminated.

