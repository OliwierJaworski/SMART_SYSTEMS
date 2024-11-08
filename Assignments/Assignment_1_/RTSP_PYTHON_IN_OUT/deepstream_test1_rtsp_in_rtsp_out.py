import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstRtspServer', '1.0')
from gi.repository import Gst, GstRtspServer, GLib

def on_pad_added(src, new_pad, depay):
    sink_pad = depay.get_static_pad("sink")
    if sink_pad.is_linked():
        return
    if new_pad.link(sink_pad) != Gst.PadLinkReturn.OK:
        print("Failed to link new pad to depay sink pad.")

def on_bus_message(bus, message, loop):
    if message.type == Gst.MessageType.ERROR:
        err, debug_info = message.parse_error()
        print(f"Error received: {err.message}")
        print(debug_info)
        loop.quit()
    elif message.type == Gst.MessageType.EOS:
        loop.quit()
    return True

def main():
    Gst.init(None)

    # Create the pipeline elements
    source = Gst.ElementFactory.make("rtspsrc", "source")
    depay = Gst.ElementFactory.make("rtph264depay", "depay")
    queue = Gst.ElementFactory.make("queue", "queue")
    decoder = Gst.ElementFactory.make("nvv4l2decoder", "decoder")
    convert = Gst.ElementFactory.make("nvvidconv", "convert")
    encoder = Gst.ElementFactory.make("nvv4l2h264enc", "encoder")
    rtppay = Gst.ElementFactory.make("rtph264pay", "rtppay")

    if not all([source, depay, queue, decoder, convert, encoder, rtppay]):
        print("Element creation failed.")
        return

    # Set properties for the source
    source.set_property("location", "rtsp://192.168.0.145:8554/xx")
    source.set_property("timeout", 30 * Gst.SECOND)
    source.set_property("latency", 2000)

    # Configure other elements
    queue.set_property("max-size-time", 10**10)
    queue.set_property("max-size-buffers", 4000)
    queue.set_property("max-size-bytes", 10**7)
    rtppay.set_property("pt", 96)
    encoder.set_property("bitrate", 2000000)
    encoder.set_property("control-rate", 2)
    encoder.set_property("iframeinterval", 30)

    # Create the pipeline and add elements
    pipeline = Gst.Pipeline.new("rtsp-in-out-pipeline")
    pipeline.add(source)
    pipeline.add(depay)
    pipeline.add(queue)
    pipeline.add(decoder)
    pipeline.add(convert)
    pipeline.add(encoder)
    pipeline.add(rtppay)

    # Link elements
    source.connect("pad-added", on_pad_added, depay)
    depay.link(queue)
    queue.link(decoder)
    decoder.link(convert)
    convert.link(encoder)
    encoder.link(rtppay)

    # Set up bus for error handling
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_bus_message, GLib.MainLoop())

    # Set the pipeline state to playing
    pipeline.set_state(Gst.State.PLAYING)

    # Set up the RTSP server
    server = GstRtspServer.RTSPServer()
    factory = GstRtspServer.RTSPMediaFactory()

    # Create the RTSP media factory and launch pipeline
    factory.set_launch("( rtspsrc location=rtsp://192.168.0.145:8554/xx ! rtph264depay ! queue ! nvv4l2decoder ! nvvidconv ! nvv4l2h264enc bitrate=4000000 control-rate=1 preset-level=1 ! rtph264pay name=pay0 pt=96 )")
    mount_points = server.get_mount_points()
    mount_points.add_factory("/output", factory)

    # Start the RTSP server
    server.attach(None)
    print("RTSP server started at rtsp://127.0.0.1:8554/output")

    # Run the main loop
    loop = GLib.MainLoop()
    loop.run()

    # Cleanup
    pipeline.set_state(Gst.State.NULL)

if __name__ == "__main__":
    main()
