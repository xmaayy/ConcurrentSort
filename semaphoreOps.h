#pragma once

int set_semvalue(int sem_id);
void del_semvalue(int sem_id);

int sem_claim(int sem_id);
int sem_release(int sem_id);
