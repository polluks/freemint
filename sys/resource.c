/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

/* 
 * resource.c - FreeMiNT. 
 * Kernel resource administration functions.
 * Author: Guido Flohr <guido@freemint.de> 
 * Started: Mon, 30 Mar 1998.
 */  

# include "resource.h"

# include "util.h"

# ifndef __MSHORT__
# error This file is not 32-bit clean.
# endif

/* NOTE:  For the rest of the *ix world a negative priority value
 *        signifies high priority when requesting cpu usage.  In
 *        the MiNT kernel things are exactly the other way round.
 *        Maybe we should fix that someday internally.  But for
 *        now the situation is handled as follows:  The usage 
 *        of the Pnice and Prenice system calls is deprecated in
 *        the presence of Pgetpriority and Psetpriority.  These
 *        two system calls will follow the standard semantics,
 *        whereas the behavior of Pnice and Prenice remains unchanged.
 *
 *        Example:  You want to give the current process top priority for
 *        cpu usage, i.e. you want to make it as fast as possible.  The
 *        current priority of the process is -10 for MiNT notion,  
 *        which is equivalent to a priority of +10 for the standard *ix notion
 *        
 *        The three possibilities to do so are:
 *
 *             Psetpriority (PRIO_PROCESS, 0, -20);
 *             Prenice (Pgetpid (), -30);
 *             Pnice (-30);
 *
 *        Provided that you have super-user privileges all possibilities
 *        will result in a (MiNT-)priority of +20 (top priority) and
 *        in a (*ix-)priority of -20 (again top priority).
 *
 *        To retrieve the current priority of a process in the standard
 *        *ix-notation you should do one of the following:
 *
 *             int priority = Pgetpriority (PRIO_PROCESS, 0);
 *             int priority = (-1) * Prenice (Pgetpid (), 0);
 *             int priority = (-1) * Pnice (0);
 *
 *        I hope that this is alright.
 */

/* GEMDOS extension:
 * Pgetpriority (which, who) gets the priority of the processes specified 
 * by WHICH and WHO. The interpretation of parameter WHO depends on WHICH:
 *
 *   - WHICH == PRIO_PROCESS 
 *     Read the priority of process with process id WHICH.  A WHO of
 *     0 implies the process id of the calling process.
 *   - WHICH == PRIO_PGRP
 *     Read the priority of the process group with process group
 *     id WHO.  If the priorities of the process differ the
 *     lowest valued priority (i. e. the highest cpu usage priority)
 *     is returned.  A WHO of 0 implies the process group id of
 *     the calling process.
 *   - WHICH == PRIO_USER
 *     Read the priority of the process of the user with user id WHO.
 *     For multiple processes the lowest valued priority is returned.
 *     A WHO of 0 implies the user id of the calling process.
 *
 * Return value is either a negative error number in case of failure
 * or the requested priority + 20 in case of failure.  Library
 * functions should first check for an error condition and then
 * decrement the returned value by 20.
 */

long _cdecl 
p_getpriority (int which, int who) 
{ 
	TRACE (("Pgetpriority: which = %d, who = %d", (int) which, (int) who));
	
	switch (which)
	{
		case PRIO_PROCESS:
		{
			PROC* p;
			if (who < 0)
			{
				DEBUG (("Pgetpriority: negative process id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
				who = curproc->pid;
			
			p = pid2proc (who);
			if (p == NULL)
			{
				DEBUG (("Pgetpriority: no such process %d", (int) who));
				return ESRCH;
			}
			
			return ((long) -(p->pri)) + PRIO_MAX;
		}

		case PRIO_PGRP:
		{
			PROC* p;
			short max_priority = PRIO_MIN;
			ulong hits = 0;
			
			if (who < 0)
			{
				DEBUG (("Pgetpriority: negative process group id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
				who = curproc->pgrp;
			
			for (p = proclist; p; p = p->gl_next)
			{
				if (p->pgrp == who)
				{
					hits++;
					if (p->pri > max_priority)
						max_priority = p->pri;  
				}
			}
			
			if (hits > 0)
			{
				return (-max_priority + PRIO_MAX);
			}
			else
			{
				DEBUG (("Pgetpriority: no process found for user id %d", (int) who));
				return ESRCH;
			}
		}
		
		case PRIO_USER:
		{
			PROC* p;
			short max_priority = PRIO_MIN;
			ulong hits = 0;
			
			if (who < 0)
			{
				DEBUG (("Pgetpriority: negative user id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
				who = curproc->euid;
			
			for (p = proclist; p; p = p->gl_next)
			{
				if (p->euid == who)
				{
					hits++;
					if (p->pri > max_priority)
						max_priority = p->pri;  
				}
			}
			
			if (hits > 0)
			{
				return (-max_priority + PRIO_MAX);
			}
			else
			{
				DEBUG (("Pgetpriority: no process found for user id %d", (int) who));
				return ESRCH;
			}
		}
	}
	
	DEBUG (("Pgetpriority: invalid opcode %d", (int) which));
	return EINVAL;
}

/* GEMDOS extension:
 * Psetpriority (WHICH, WHO, PRI) sets the priority of the processes 
 * specified by WHICH and WHO.
 * The interpretation of parameters WHO and WHICH are analog to
 * Pgetpriority.  The third argument is the new priority to assign
 * (not the increment but the absolute value).  The PRI argument
 * is silently changed to the maximum (resp. minimum) possible
 * value if it is not in the range between PRIO_MIN and PRIO_MAX.
 *
 * The function returns 0 for success or a negative error code
 * for failure.  The following error conditions are defined:
 *
 *   EINVAL - Invalid argument to WHO, WHICH or PRI.
 *   EACCES - The calling process is not owner of one or more
 *            of the selected processes.  The other selected
 *            processes are still affected.
 *   EPERM  - The calling process does not have the privilege
 *            to change the priority of one or more of the selected
 *            processes.  This can only happen if an attempt
 *            was made to change the priority of a process to
 *            a positive value.
 *   ESRCH  - The combination of WHICH and WHO does not match
 *            an existing process.
 *
 * The error condition reported is the last error condition 
 * encountered (in other words if both EACCES and EPERM occur
 * the return value is arbitrary).
 */
 
long _cdecl 
p_setpriority (int which, int who, int pri)
{ 
	TRACE (("Psetpriority: which = %d, who = %d, pri = %d", (int) which, (int) who, (int) pri));

	if (pri < PRIO_MIN)
		pri = PRIO_MIN;
	else if (pri > PRIO_MAX)
		pri = PRIO_MAX;
  
	if (pri < 0 && curproc->euid != 0)
	{
		DEBUG (("Psetpriority: attempt to assign negative priority by non-root"));
		return EPERM;
	}
	
	switch (which)
	{
		case PRIO_PROCESS:
		{
			PROC* p;
			
			if (who < 0)
			{
				DEBUG (("Psetpriority: negative process id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
				who = curproc->pid;
			
			p = pid2proc (who);
			if (p == NULL)
			{
				DEBUG (("Psetpriority: no such process %d", (int) who));
				return ESRCH;
			}
			if (curproc->euid != 0 && curproc->euid != p->euid)
			{
				DEBUG (("Psetpriority: not owner"));
				return EACCES;
			}
			p->pri = p->curpri = -pri;
			return E_OK;
		}
		
		case PRIO_PGRP:
		{
			PROC* p;
			ulong hits = 0;
			ulong retval = 0;
			
			if (who < 0)
			{
				DEBUG (("Psetpriority: negative process group id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
				who = curproc->pgrp;
			
			for (p = proclist; p; p = p->gl_next)
			{
				if (p->pgrp == who) 
					hits++;
				
				if (curproc->euid != 0 && curproc->euid != p->euid)
				{
					DEBUG (("Psetpriority: not owner"));
					retval = EACCES;
				}
				else
				{
					p->pri = p->curpri = -pri;
				}
			}
			
			if (hits == 0)
			{
				DEBUG (("Psetpriority: no process found for user id %d", (int) who));
				return ESRCH;
			}
			
			return retval;
		}
		
		case PRIO_USER:
		{
			PROC* p;
			ulong hits = 0;
			ulong retval = 0;
			
			if (who < 0)
			{
				DEBUG (("Psetpriority: negative user id %d", (int) who));
				return EINVAL;
			}
			else if (who == 0)
			{
				who = curproc->euid;
			}
			
			if (who != curproc->euid)
			{
				DEBUG (("Psetpriority: not owner"));
				retval = EACCES;
			}
			
			for (p = proclist; p; p = p->gl_next)
			{
				if (p->euid == who)
				{
					hits++;
					p->pri = p->curpri = -pri;
				}
			}
			
			if (hits == 0)
			{
				DEBUG (("Psetpriority: no process found for user id %d", (int) who));
				return ESRCH;
			}
			
			return retval;
		}
	}
	
	DEBUG (("Psetpriority: invalid opcode %d", (int) which));
	return EINVAL;
}

/*
 * GEMDOS extension:
 * Prenice(pid, delta) sets the process priority level for process pid.
 * A "nice" value < 0 increases priority, one > 0 decreases it.
 * Always returns the new priority (so Prenice(pid, 0) queries the current
 * priority).
 *
 * NOTE: for backward compatibility, Pnice(delta) is provided and is equivalent
 * to Prenice(Pgetpid(), delta).
 *
 * 30 Mar 1998: The semantics have slightly changed.  Read access is
 * now granted unlimitedly.  This is standard and shouldn't be considered
 * as a security hole.  By contrary, it is now reserved to the super-user
 * to increase any processes priority to a positive value (remember that
 * for MiNT a positive priority increases the chance to get a time slice).
 * 
 * Anyway, the use of Pnice and Prenice is deprecated now.  Use Pgetpriority
 * or Psetpriority instead.
 */

long _cdecl
p_renice (int pid, int delta)
{
	long pri;
	
	if (pid < 0)
		return ESRCH;
	
	pri = p_getpriority (PRIO_PROCESS, pid);
	
	if (pri < 0)
		return pri;
	
	pri -= PRIO_MAX;  /* Mask out error code.  */
	
	if (delta != 0)
	{
		long r;
		
		pri += delta;
		
		r = p_setpriority (PRIO_PROCESS, pid, (int) pri);
		if (r < E_OK)
			return r;
	}
	
	/* The variable PRI has now standard meaning.
	 * Change it back.
	 */
	pri *= -1;
	
	/* Strange enough but that was the way the
	 * old code worked.
	 */
	return (pri & 0x0ffff);
}

long _cdecl
p_nice (int delta)
{
	return p_renice (curproc->pid, delta);
}
