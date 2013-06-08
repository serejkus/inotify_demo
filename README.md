inotify_demo
============

Demo of inotify in linux.
Nothing useful, just pass by.

Main steps of inotify use
=========================

1. Create inotify instance using inotify_init() or inotify_init1() (non-standard).
2. Add files using inotify_add_watch() (not recursive for directories). You get a watch descriptor, which is used to refer to the watch in later operations. To remove a watch use inotify_rm_watch().
3. read() from inotify descriptor. You get one or more inotify_event structures.
4. Close inotify file descriptor. It also removes all watch items associated with inotify instance.

Configure
---------

* /proc/sys/fs/inotify/max_queued_events - upper limit on the number of events that can be queued to inotify instance. On exess elemnts are dropped, IN_Q_OVERFLOW flag is set;
* /proc/sys/fs/inotify/max_user_instances - upper limit on the number of inotify instances that can be created per real user ID;
* /proc/sys/fs/inotify/max_user_watches - upper limit on the number of watches that can be created per real user ID.

