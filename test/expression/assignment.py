_prog.run(lambda r,v: v.expecteq(r.exitcode, 80)) # argc = 1
_prog.run(lambda r,v: v.expecteq(r.exitcode, 130), ['a']) # argc = 2
_prog.run(lambda r,v: v.expecteq(r.exitcode, 180), ['a', 'a']) # argc = 3

