package cloud.lesh.CPUSim64v2;

public class Utils {
	public static double asinh(double x) {
		return Math.log(x + Math.sqrt(x * x + 1.0));
	}

	public static double acosh(double x) {
		return Math.log(x + Math.sqrt(x * x - 1.0));
	}

	public static double atanh(double x) {
		return 0.5 * Math.log((1.0 + x) / (1.0 - x));
	}
}
