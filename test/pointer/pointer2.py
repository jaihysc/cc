_prog.run(lambda r,v: v.expecteq(r.exitcode, 99))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 99), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 99), ['a', 'a'])

