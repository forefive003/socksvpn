#ifndef _JOB_IO_MGR_H
#define _JOB_IO_MGR_H


typedef std::list<CIoJob*> IOJOB_LIST;
typedef IOJOB_LIST::iterator IOJOB_LIST_Itr;

class CIoJobMgr
{
public:
	CIoJobMgr();
    virtual ~CIoJobMgr();
    static CIoJobMgr* instance()
    {
        static CIoJobMgr *ioJobMgr = NULL;

        if(ioJobMgr == NULL)
        {
            ioJobMgr = new CIoJobMgr();
        }
        return ioJobMgr;
    }

public:
	CIoJob* find_io_job(int fd);
	void add_io_job(CIoJob* ioJob);
	void del_io_job(CIoJob* ioJob);
	void move_to_deling_job(CIoJob* ioJob);

	void lock();
	void unlock();

	int walk_to_set_sets(fd_set &rset, fd_set &wset);
	void walk_to_handle_sets(fd_set &rset, fd_set &wset);
	void handle_deling_job(unsigned int thrd_index);
private:
	MUTEX_TYPE m_job_lock;
	IOJOB_LIST m_io_jobs;
	IOJOB_LIST m_del_io_jobs;
};

extern CIoJobMgr *g_IoJobMgr;

#endif
