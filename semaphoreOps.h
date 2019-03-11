#pragma once

int set_semvalue(int sem_id);
void del_semvalue(int sem_id);

int semaphore_p(int sem_id);
int semaphore_v(int sem_id);