#ifndef __MEM_H__
#define __MEM_H__

void Q_InitMem( void );
void Q_PrintMem( void );
void Q_ShutdownMem( qboolean printstats );

void *qmalloc( size_t size );
void qfree( void *data );

#endif