#include "main.h"

UNICODE_STRING  sym_link, dev_name;

NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp)
{
	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_obj);
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS ctl_io(PDEVICE_OBJECT device_obj, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = sizeof(info);

	auto stack = IoGetCurrentIrpStackLocation(irp);
	auto buffer = (p_info)irp->AssociatedIrp.SystemBuffer;

	size_t size = 0;
	if (stack)
	{
		if (buffer && sizeof(*buffer) >= sizeof(info))
		{
			if (stack->Parameters.DeviceIoControl.IoControlCode == ctl_read)
			{
				if (buffer->address < 0x7FFFFFFFFFFF)
				{
					core::read_memory(buffer->process_id, (void*)buffer->address, buffer->value, buffer->size);
				}
				else
				{
					buffer->value = nullptr;
				}
			}
			else if (stack->Parameters.DeviceIoControl.IoControlCode == ctl_write)
			{
				core::write_memory(buffer->process_id, (void*)buffer->address, buffer->value, buffer->size);
			}
			else if (stack->Parameters.DeviceIoControl.IoControlCode == ctl_alloc)
			{
				core::AllocMemory(buffer->process_id, (void*)buffer->address, buffer->size, buffer->Protect);
			}
			else if (stack->Parameters.DeviceIoControl.IoControlCode == ctl_free)
			{
				core::FreeMemory(buffer->process_id, (void*)buffer->address);
			}
		}
	}
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

void UnloadDriver(IN PDRIVER_OBJECT pDriverObject);

NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path)
{
	PDEVICE_OBJECT  dev_obj;

	RtlInitUnicodeString(&dev_name, L"\\Device\\keboost");
	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\keboost");
	IoCreateDevice(driver_obj, 0, &dev_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);

	IoCreateSymbolicLink(&sym_link, &dev_name);


	dev_obj->Flags |= DO_BUFFERED_IO;

	for (int t = 0; t <= IRP_MJ_MAXIMUM_FUNCTION; t++)
		driver_obj->MajorFunction[t] = unsupported_io;

	driver_obj->MajorFunction[IRP_MJ_CREATE] = create_io;
	driver_obj->MajorFunction[IRP_MJ_CLOSE] = close_io;
	driver_obj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ctl_io;
	//driver_obj->DriverUnload = UnloadDriver;


	dev_obj->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

NTSTATUS FakeEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registery_path)
{
	UNICODE_STRING  drv_name;

	RtlInitUnicodeString(&drv_name, L"\\Driver\\keboost");
	IoCreateDriver(&drv_name, &DriverEntry);

	return STATUS_SUCCESS;
}

void UnloadDriver(IN PDRIVER_OBJECT pDriverObject)
{
	IoDeleteSymbolicLink(&sym_link);
	IoDeleteDevice(pDriverObject->DeviceObject);
}