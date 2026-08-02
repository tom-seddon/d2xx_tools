#!/usr/bin/python3
import sys,os,os.path,argparse,shutil,collections,subprocess,shlex

##########################################################################
##########################################################################

g_verbose=False

def pv(msg):
    if g_verbose:
        sys.stdout.write(msg)
        sys.stdout.flush()

##########################################################################
##########################################################################

def fatal(msg):
    sys.stderr.write('FATAL: %s\n'%msg)
    sys.exit(1)

##########################################################################
##########################################################################

def is_macos(): return sys.platform=='darwin'

def is_windows(): return sys.platform=='win32'

def is_linux(): return sys.platform=='linux'

def is_unix(): return is_macos() or is_linux()

##########################################################################
##########################################################################

def makedirs(path):
    if not os.path.isdir(path):
        pv(f'''b2build mkdir: {path}\n''')
        os.makedirs(path)

def rmtree(path):
    if os.path.isdir(path):
        pv(f'''b2build rmtree: {path}\n''')
        shutil.rmtree(path)

##########################################################################
##########################################################################

class ChangeDirectory:
    def __init__(self,path):
        self._oldcwd=os.getcwd()
        self._newcwd=path

        pv('b2build ChangeDirectory was: %s\n'%self._oldcwd)
        pv('b2build ChangeDirectory now: %s\n'%self._newcwd)

    def __enter__(self):
        os.chdir(self._newcwd)
        return self

    def __exit__(self,*args): os.chdir(self._oldcwd)

    def relpath(self,path): return os.path.relpath(path,self._newcwd)

    def join(self,path): return os.path.join(self._newcwd,path)

##########################################################################
##########################################################################

def get_copyable_argv(argv):
    assert isinstance(argv,list),type(argv)

    def quote(x):
        assert isinstance(x,str),type(x)
    
        if is_windows():
            if not x.startswith('"') and ' ' in x: return '"%s"'%x
            else: return x
        else: return shlex.quote(x)
        
    return ' '.join([quote(arg) for arg in argv])

##########################################################################
##########################################################################

VISUAL_STUDIO_VERSIONS_BY_YEAR={
    2022:17,
}

VISUAL_STUDIO_DEFAULT_YEAR=2022

##########################################################################
##########################################################################

VSStuff=collections.namedtuple('VSStuff','year version install_path devenv_path cmake_path ctest_path')

def get_vs_stuff(year):
    version=VISUAL_STUDIO_VERSIONS_BY_YEAR.get(year)
    assert version

    argv=[r'''C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe''',
          '-version',str(version),
          '-property','installationPath']
    vswhere_subprocess=subprocess.Popen(argv,
                                            stdout=subprocess.PIPE,
                                            stderr=subprocess.DEVNULL)
    vswhere_subprocess.wait()
    if vswhere_subprocess.returncode!=0:
        fatal('failed with exit code %d: %s'%(ret.returncode,get_copyable_argv(argv)))

    # TODO: do you need to specify an encoding here?
    data=vswhere_subprocess.stdout.read()
    install_path=data.strip().decode()

    cmake_bin_path=os.path.join(install_path,
                                'common7',
                                'IDE',
                                'CommonExtensions',
                                'Microsoft',
                                'CMake',
                                'CMake',
                                'bin')

    def get_required_file_path(*parts):
        path=os.path.join(*parts)
        if not os.path.isfile(path): fatal('file not found: %s'%path)
        return path

    cmake_path=get_required_file_path(cmake_bin_path,'cmake.exe')
    ctest_path=get_required_file_path(cmake_bin_path,'ctest.exe')
    devenv_path=get_required_file_path(install_path,'Common7/IDE/devenv.com')

    return VSStuff(year=year,
                   version=version,
                   install_path=install_path,
                   devenv_path=devenv_path,
                   cmake_path=cmake_path,
                   ctest_path=ctest_path)

##########################################################################
##########################################################################

FakeProcess=collections.namedtuple('FakeProcess','returncode')

def run_subprocess(argv,options,execute=True,**other_popen_kwargs):
    argv=[arg for arg in argv if arg is not None]

    suffix='(cwd: %s): %s'%(os.getcwd(),get_copyable_argv(argv))

    if execute:
        pv(f'b2build running   {suffix}\n')
            
        process=subprocess.Popen(argv,**other_popen_kwargs)
        process.wait()

        pv(f'b2build completed {suffix} - exit code: {process.returncode}\n')
    else:
        # return process with error exit code. If the caller doesn't
        # check: no problem!
        process=FakeProcess(returncode=1)
        pv(f'b2build running   {suffix} - fake exit code: {process.returncode}\n')
        
    return process

##########################################################################
##########################################################################

def main2(options):
    global g_verbose
    g_verbose=options.verbose

    def add_cmake_version_string_arguments(argv):
        if options.version_string is not None:
            argv.append('-DVERSION_STRING='+options.version_string)
    
    if is_windows():
        vsyear=None
        for year in sorted(VISUAL_STUDIO_VERSIONS_BY_YEAR.keys()):
            if getattr(options,'vs%d'%year,False):
                if vsyear is not None:
                    fatal('only one --vsXXXX option may be provided')
                vsyear=year

        if vsyear is None: vsyear=VISUAL_STUDIO_DEFAULT_YEAR
        vs_stuff=get_vs_stuff(vsyear)

        root_output_path=options.output_path
        if root_output_path is None: root_output_path='build'

        def cmake_vs(arch):
            output_path=os.path.join(root_output_path,
                                     f'''vs{vs_stuff.year}.{arch}''')

            if options.clean: rmtree(output_path)
            makedirs(output_path)

            argv=[vs_stuff.cmake_path,
                  '-G',f'''Visual Studio {vs_stuff.version} {vs_stuff.year}''',
                  '-A',arch,
                  '-S','.',
                  '-B',output_path]
            add_cmake_version_string_arguments(argv)

            ret=run_subprocess(argv,options,close_fds=False)
            if ret.returncode!=0: fatal('init failed')

        cmake_vs('x64')
        cmake_vs('Win32')

    elif is_macos():
        output_path=options.output_path
        if output_path is None: output_path='build/Xcode'

        if options.clean: rmtree(output_path)
        makedirs(output_path)

        argv=['cmake',
              '-G','Xcode',
              '-S','.',
              '-B',output_path]
        add_cmake_version_string_arguments(argv)

        ret=run_subprocess(argv,options,close_fds=False)
        if ret.returncode!=0: fatal('init failed')
    else:
        fatal('unsupported OS')

##########################################################################
##########################################################################
    
def main(argv):
    parser=argparse.ArgumentParser()

    def auto_int(x): return int(x,0)

    if is_windows():
        for year in sorted(VISUAL_STUDIO_VERSIONS_BY_YEAR.keys()):
            help=f'''use Visual Studio {year}'''
            if year==VISUAL_STUDIO_DEFAULT_YEAR:
                help+=''' (default if none specified)'''
            parser.add_argument('--vs%d'%year,action='store_true',help=help)

    parser.add_argument('--verbose',action='store_true',help='''be more verbose''')
    parser.add_argument('--clean',action='store_true',help='''clean output folder (no questions asked!) before configuring''')
    parser.add_argument('--version-string',metavar='STR',help='''set version string to %(metavar)s''')
    parser.add_argument('-o',dest='output_path',default=None,help='''path to output folder''')

    main2(parser.parse_args(argv))

if __name__=='__main__': main(sys.argv[1:])
