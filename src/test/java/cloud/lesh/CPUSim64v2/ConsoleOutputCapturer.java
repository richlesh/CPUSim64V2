package cloud.lesh.CPUSim64v2;

/**
 * From https://stackoverflow.com/questions/4334808/how-could-i-read-java-console-output-into-a-string-buffer
 */
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.util.Arrays;
import java.util.List;

public class ConsoleOutputCapturer {
    private ByteArrayOutputStream baos;
	private PrintStream previousOut;
	private PrintStream previousErr;
    private boolean capturing = false;

	public enum StdStream { STDOUT, STDERR, BOTH}

    public void start(StdStream which) {
        if (capturing) {
            return;
        }

        capturing = true;
		previousOut = System.out;
		previousErr = System.err;

		baos = new ByteArrayOutputStream();

		if (which == StdStream.STDOUT) {
			OutputStream outputStreamCombiner =
					new OutputStreamCombiner(Arrays.asList(previousOut, baos));
			PrintStream custom = new PrintStream(outputStreamCombiner);
			System.setOut(custom);
		} else if (which == StdStream.STDERR) {
			OutputStream outputStreamCombiner =
					new OutputStreamCombiner(Arrays.asList(previousErr, baos));
			PrintStream custom = new PrintStream(outputStreamCombiner);
			System.setErr(custom);
		} else {
			OutputStream outputStreamCombiner1 =
					new OutputStreamCombiner(Arrays.asList(previousOut, baos));
			PrintStream custom1 = new PrintStream(outputStreamCombiner1);
			System.setOut(custom1);
			OutputStream outputStreamCombiner2 =
					new OutputStreamCombiner(Arrays.asList(previousErr, baos));
			PrintStream custom2 = new PrintStream(outputStreamCombiner2);
			System.setErr(custom2);
		}
    }

    public String stop() {
        if (!capturing) {
            return "";
        }

		System.setOut(previousOut);
		System.setErr(previousErr);

        String capturedValue = baos.toString();             

        baos = null;
		previousOut = null;
		previousErr = null;
        capturing = false;

        return capturedValue;
    }

    private static class OutputStreamCombiner extends OutputStream {
        private List<OutputStream> outputStreams;

        public OutputStreamCombiner(List<OutputStream> outputStreams) {
			this.outputStreams = outputStreams;
        }

        public void write(int b) throws IOException {
            for (OutputStream os : outputStreams) {
                os.write(b);
            }
        }

        public void flush() throws IOException {
            for (OutputStream os : outputStreams) {
                os.flush();
            }
        }

        public void close() throws IOException {
            for (OutputStream os : outputStreams) {
                os.close();
            }
        }
    }
}