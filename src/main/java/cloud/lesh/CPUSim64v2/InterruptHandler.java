package cloud.lesh.CPUSim64v2;

public abstract class InterruptHandler
{
	protected InterruptHandler superHandler;

	public abstract boolean dispatch(int id) throws Simulator.CPUException;
	public void setSuperHandler(InterruptHandler h) {superHandler=h;}
}
