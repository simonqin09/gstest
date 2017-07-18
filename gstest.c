#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <stdbool.h>

//message processing
static gboolean bus_call(GstBus * bus, GstMessage * msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE(msg))
    {

        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
            {
                gchar *debug;
                GError *error;
                gst_message_parse_error(msg, &error, &debug);
                g_free(debug);
                g_printerr("ERROR:%s\n", error->message);
                g_error_free(error);
                g_main_loop_quit(loop);
                break;
            }
        default:
            break;
    }

    return 1;
}

on_pad_added (GstElement *element, GstPad *pad, gpointer data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;
    /* We can now link this pad with the vorbis-decoder sink pad */
    g_print ("Dynamic pad created, linking demux/decoder\n");
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
}

int main (int argc, char *argv[])
{
	//create element, caps and bus
	GMainLoop *loop;
    GstElement *pipeline, *source, *converter, *encoder, *decoder, *demux, *mux, *sink, *vqueue;
    GstElement *source_capsfilter;
    GstCaps *source_caps;
    GstBus *bus;

    //initial gstreamer
	gst_init (&argc, &argv);

	//create main loop, start to loop after running g_main_loop_run
	loop = g_main_loop_new(NULL, -1);
	if ((argc < 3)||(argc > 4))
	{
	    g_printerr("Usage:%s [1] [camera device name] for camera capture or\n", argv[0]);
	    g_printerr("Usage:%s [2] [camera device name] [video storage location] for camera video record or\n", argv[0]);
	    g_printerr("Usage:%s [3] [video file location] for video file play\n", argv[0]);
	    return -1;
	}
    int option = atoi(argv[1]);
    if(option == 1){
    	/* sample code for capture USB camera */
    		//create pipeline and element
    		pipeline = gst_pipeline_new("usb-camera");
    		source = gst_element_factory_make("imxv4l2src", "camera-input");
    		// set source parameters
    		g_object_set(G_OBJECT(source),"device",argv[2],NULL);
    	    // create source caps filter
    		source_capsfilter = gst_element_factory_make("capsfilter", "source_capsfilter");
    		source_caps = gst_caps_new_simple ("video/x-raw",
    				   "format", G_TYPE_STRING, "YUY2",
    				   "width", G_TYPE_INT, 1280,
    				   "height", G_TYPE_INT, 720,
    				   NULL);
    		g_object_set(G_OBJECT(source_capsfilter),"caps", source_caps, NULL);
    	    // create converter element
    		converter = gst_element_factory_make("videoconvert", "video-converter");
    		// create sink element
    		sink = gst_element_factory_make("imxv4l2sink", "camera-output");
    		g_object_set(G_OBJECT(sink),"overlay-width", 640, NULL);
    		g_object_set(G_OBJECT(sink),"overlay-height", 480, NULL);

    		if (!pipeline || !source || !source_capsfilter || !converter || !sink)
    		{
    		   g_printerr("One element could not be created.Exiting.\n");
    		   return -1;
    		}
    		// create pipeline message bus
    		bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    		// add message monitor
    		gst_bus_add_watch(bus, bus_call, loop);
    		gst_object_unref(bus);

    		// add elements to pipeline
    		gst_bin_add_many(GST_BIN(pipeline), source, source_capsfilter, converter, sink, NULL);
    	    // connect elements sequentially
    		gst_element_link_many(source, source_capsfilter, converter, sink, NULL);
    }
    else if(option == 2){
    	/* sample code for recording USB camera */
    	//create pipeline and element
    		pipeline = gst_pipeline_new("usb-camera");
    		source = gst_element_factory_make("imxv4l2src", "camera-input");
    		// set source parameters
    		g_object_set(G_OBJECT(source),"device",argv[2],NULL);
    	    // create source caps filter
    		source_capsfilter = gst_element_factory_make("capsfilter", "source_capsfilter");
    		source_caps = gst_caps_new_simple ("video/x-raw",
    				   "format", G_TYPE_STRING, "YUY2",
    				   "width", G_TYPE_INT, 1280,
    				   "height", G_TYPE_INT, 720,
    				   NULL);
    		g_object_set(G_OBJECT(source_capsfilter),"caps", source_caps, NULL);
    	    // create converter element
    		converter = gst_element_factory_make("videoconvert", "video-converter");
    		// create H264 encoder element
    		encoder = gst_element_factory_make("vpuenc_h264", "video-encoder");
    		// create H264 MUX element
    		mux = gst_element_factory_make("matroskamux", "video-mux");
    		// create sink element
    		sink = gst_element_factory_make("filesink", "file-storage");
    		g_object_set(G_OBJECT(sink),"location", argv[3], NULL);

    		if (!pipeline || !source || !source_capsfilter || !converter || !encoder || !mux || !sink)
    		{
    		   g_printerr("One element could not be created.Exiting.\n");
    		   return -1;
    		}
    		// create pipeline message bus
    		bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    		// add message monitor
    		gst_bus_add_watch(bus, bus_call, loop);
    		gst_object_unref(bus);

    		// add elements to pipeline
    		gst_bin_add_many(GST_BIN(pipeline), source, source_capsfilter, converter, encoder, mux, sink, NULL);
    		// connect elements sequentially
    		gst_element_link_many(source, source_capsfilter, converter, encoder, mux, sink, NULL);
    }
    else if(option == 3){
    	/* sample code for playing recorded video */
    	    bool flag = true;
    	    //create pipeline and element
    	    pipeline = gst_pipeline_new("usb-camera");
    	    source = gst_element_factory_make("filesrc", "file-input");
    	    // set source parameters
    	    g_object_set(G_OBJECT(source),"location",argv[2],NULL);
    	    g_object_set(G_OBJECT(source),"typefind",flag,NULL);
    		// create H264 DEMUX element
        	demux = gst_element_factory_make("matroskademux", "video-demux");
    	    // create H264 decoder element
    	    decoder = gst_element_factory_make("vpudec", "video-decoder");

    	    // create sink element
    	    sink = gst_element_factory_make("imxv4l2sink", "video-play");
    	    g_object_set(G_OBJECT(sink),"overlay-width", 640, NULL);
    	    g_object_set(G_OBJECT(sink),"overlay-height", 480, NULL);

    	    if (!pipeline || !source || !demux || !decoder || !sink)
    	    {
    	    	g_printerr("One element could not be created.Exiting.\n");
    	    	return -1;
    	    }
    	    // create pipeline message bus
    	    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    	    // add message monitor
    	    gst_bus_add_watch(bus, bus_call, loop);
    	    gst_object_unref(bus);

    	    // add elements to pipeline
    	    gst_bin_add_many(GST_BIN(pipeline), source, demux, decoder, sink, NULL);
    	    // connect elements
    	    /* note that the demux will be linked to the decoder dynamically.
    	    The reason is that mkv file may contain various streams (for example
    	    audio and video). The source pad(s) will be created at run time
    	    by the demux when it detects the amount and nature of streams.
    	    Therefore we connect a callback function which will be executed
    	    when the "pad-added" is emitted.*/
    	    gst_element_link(source, demux);
    	    gst_element_link_many(decoder, sink, NULL);
    	    g_signal_connect(demux, "pad-added", G_CALLBACK (on_pad_added), decoder);
    }
    else{
    	g_printerr("wrong parameter input\n");
    	return -1;
    }

	// star to play
	gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_print("Running\n");

	// start to loop
	g_main_loop_run(loop);

	// quit loop and return
	g_print("Returned,stopping playback\n");
	gst_element_set_state(pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(pipeline));

	return 0;
}
