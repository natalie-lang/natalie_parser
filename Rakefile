task default: :build

desc 'Build Natalie Parser library and MRI C extension'
task build: %i[
  bundle_install
  build_dir
  library
  parser_c_ext
  write_compile_database
]

so_ext = RUBY_PLATFORM =~ /darwin/ ? 'bundle' : 'so'

desc 'Build Natalie Parser library'
task library: "build/libnatalie_parser.a"

desc 'Build Natalie Parser MRI C extension'
task parser_c_ext: "ext/natalie_parser/natalie_parser.#{so_ext}"

desc 'Remove temporary files created during build'
task :clean do
  Rake::FileList[%w[
    build/build.log
    build/*.o
    ext/natalie_parser/*.{h,log,o}
  ]].each { |path| rm_rf path if File.exist?(path) }
end

desc 'Remove all generated files'
task :clobber do
  Rake::FileList[%w[
    build
    ext/natalie_parser/*.{so,bundle,h,log,o}
  ]].each { |path| rm_rf path if File.exist?(path) }
end

task distclean: :clobber

desc 'Run the test suite'
task test: :build do
  sh 'bundle exec ruby test/all.rb'
end

desc 'Show line counts for the project'
task :cloc do
  sh 'cloc include lib src test'
end

desc 'Generate tags file for development'
task :ctags do
  sh 'ctags -R --exclude=.cquery_cache --exclude=ext --exclude=build --append=no .'
end
task tags: :ctags

desc 'Format C++ code with clang-format'
task :format do
  sh "find include -type f -name '*.hpp' -exec clang-format -i --style=file {} +"
  sh "find src -type f -name '*.cpp' -exec clang-format -i --style=file {} +"
end

desc 'Show TODO and FIXME comments in the project'
task :todo do
  sh "egrep -r 'FIXME|TODO' src include lib"
end

# # # # Docker Tasks (used for CI) # # # #

DOCKER_FLAGS =
  if !ENV['CI'] && STDOUT.isatty
    '-i -t'
  elsif ENV['CI']
    "-e CI=#{ENV['CI']}"
  end

task :docker_build do
  sh 'docker build -t natalie-parser .'
end

task docker_bash: :docker_build do
  sh 'docker run -it --rm --entrypoint bash natalie-parser'
end

task :docker_build_clang do
  sh 'docker build -t natalie-parser-clang --build-arg CC=clang --build-arg CXX=clang++ .'
end

task :docker_build_ruby27 do
  sh 'docker build -t natalie-parser-ruby27 --build-arg IMAGE="ruby:2.7" .'
end

task docker_test: %i[docker_test_gcc docker_test_clang docker_test_ruby27]

task docker_test_gcc: :docker_build do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie-parser test"
end

task docker_test_clang: :docker_build_clang do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie-parser-clang test"
end

task docker_test_ruby27: :docker_build_ruby27 do
  sh "docker run #{DOCKER_FLAGS} --rm --entrypoint rake natalie-parser-ruby27 test"
end

# # # # Build Compile Database # # # #

if system('which compiledb 2>&1 >/dev/null')
  $compiledb_out = []

  def $stderr.puts(str)
    write(str + "\n")
    $compiledb_out << str
  end

  task :write_compile_database do
    if $compiledb_out.any?
      File.write('build/build.log', $compiledb_out.join("\n"))
      sh 'compiledb < build/build.log'
    end
  end
else
  task :write_compile_database do
    # noop
  end
end

# # # # Internal Tasks and Rules # # # #

STANDARD = 'c++17'
HEADERS = Rake::FileList['include/**/{*.h,*.hpp}']
SOURCES = Rake::FileList['src/**/*.{c,cpp}']
OBJECT_FILES = SOURCES.sub('src/', 'build/').pathmap('%p.o')

require 'tempfile'

task :build_dir do
  mkdir_p 'build/parser_c_ext' unless File.exist?('build/parser_c_ext')
end

rule '.cpp.o' => ['src/%n'] + HEADERS do |t|
  sh "#{cxx} #{cxx_flags.join(' ')} -std=#{STANDARD} -c -o #{t.name} #{t.source}"
end

file 'build/libnatalie_parser.a' => HEADERS + OBJECT_FILES do |t|
  sh "ar rcs #{t.name} #{OBJECT_FILES}"
end

file "ext/natalie_parser/natalie_parser.#{so_ext}" => ['build/libnatalie_parser.a', 'ext/natalie_parser/natalie_parser.cpp'] do |t|
  build_dir = File.expand_path('ext/natalie_parser', __dir__)
  sh <<-SH
    cd #{build_dir} && \
    ruby extconf.rb && \
    make
  SH
end

task :bundle_install do
  sh 'bundle check || bundle install'
end

def cc
  @cc ||=
    if ENV['CC']
      ENV['CC']
    elsif system('which ccache 2>&1 > /dev/null')
      'ccache cc'
    else
      'cc'
    end
end

def cxx
  @cxx ||=
    if ENV['CXX']
      ENV['CXX']
    elsif system('which ccache 2>&1 > /dev/null')
      'ccache c++'
    else
      'c++'
    end
end

def cxx_flags
  base_flags =
    case ENV['BUILD']
    when 'release'
      %w[
        -fPIC
        -g
        -O2
      ]
    else
      %w[
        -fPIC
        -g
        -Wall
        -Wextra
        -Werror
      ]
    end
  base_flags + include_paths.map { |path| "-I #{path}" }
end

def include_paths
  [File.expand_path('include', __dir__)]
end
