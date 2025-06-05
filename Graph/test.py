def print_task_map():
    print("Task Name     State   Prio  Stack  Num");
    print("---------------------------------------");
    print("IDLE0         R       0     2048   0");
    print("IDLE1         R       0     2048   1");
    print("Tmr Svc       B       1     3120   0");
    print("main_task     R       5     2400   1");
    print("control_task  R       4     2048   0");
    print("mqtt_task     B       3     1824   1");
    print("sensor_task   B       3     1760   0");
    print("ota_task      B       2     1632   1");
    print("telemetry_tx  B       2     1520   1\n");

    print("Name: control_task,    Priority: 4, Core: 0");
    print("Name: mqtt_task,       Priority: 3, Core: 1");
    print("Name: sensor_task,     Priority: 3, Core: 0");
    print("Name: ota_task,        Priority: 2, Core: 1");
    print("Name: telemetry_tx,    Priority: 2, Core: 1");
    print("Name: Tmr Svc,         Priority: 1, Core: 0");
    print("Name: IDLE0,           Priority: 0, Core: 0");
    print("Name: IDLE1,           Priority: 0, Core: 1");
    print("Name: main_task,       Priority: 5, Core: 1");

print_task_map()