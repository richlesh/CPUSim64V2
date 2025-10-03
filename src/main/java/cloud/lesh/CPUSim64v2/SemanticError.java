package cloud.lesh.CPUSim64v2;

public final class SemanticError {
	public final int line;
	public final int col;
	public final String message;

	public SemanticError(int line, int col, String message) {
		this.line = line;
		this.col = col;
		this.message = message;
	}

	@Override public String toString() {
		return "line " + line + ":" + col + " " + message;
	}
}
