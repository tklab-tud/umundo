package org.umundo.rpc;

public interface IServiceListener {
	void addedService(ServiceDescription desc);
	void removedService(ServiceDescription desc);
	void changedService(ServiceDescription desc);
}
