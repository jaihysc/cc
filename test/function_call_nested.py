_prog.run(lambda r,v: v.expecteq(r.exitcode, 46))
_prog.run(lambda r,v: v.expecteq(r.exitcode, 47), ['a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 48), ['a', 'a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 49), ['a', 'a', 'a'])
_prog.run(lambda r,v: v.expecteq(r.exitcode, 50), ['a', 'a', 'a', 'a'])
