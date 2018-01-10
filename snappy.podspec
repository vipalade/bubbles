#
#  Be sure to run `pod spec lint CuteAndCuddly.podspec' to ensure this is a
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see http://docs.cocoapods.org/specification.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

Pod::Spec.new do |s|

  # ―――  Spec Metadata  ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  These will help people to find your library, and whilst it
  #  can feel like a chore to fill in it's definitely to your advantage. The
  #  summary should be tweet-length, and the description more in depth.
  #

  s.name         = "snappy"
  s.version      = "1.1.7"
  s.summary      = "Google Snappy compression/decompression library"

  # This description is used to generate tags and improve search results.
  #   * Think: What does it do? Why did you write it? What is the focus?
  #   * Try to keep it short, snappy and to the point.
  #   * Write the description between the DESC delimiters below.
  #   * Finally, don't worry about the indent, CocoaPods strips it!
  s.description  = <<-DESC
    Google Snappy compression/decompression library.
                   DESC

  s.homepage     = "https://github.com/google/snappy.git"

  # ―――  Spec License  ――――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  Licensing your code is important. See http://choosealicense.com for more info.
  #  CocoaPods will detect a license file if there is a named LICENSE*
  #  Popular ones are 'MIT', 'BSD' and 'Apache License, Version 2.0'.
  #

  s.license      = { :type => 'MIT', :file => 'COPYING' }

  # ――― Author Metadata  ――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  Specify the authors of the library, with email addresses. Email addresses
  #  of the authors are extracted from the SCM log. E.g. $ git log. CocoaPods also
  #  accepts just a name if you'd rather not provide an email address.
  #
  #  Specify a social_media_url where others can refer to, for example a twitter
  #  profile URL.
  #

  s.author             = { "Valentin Palade" => "vipalade@gmail.com" }

  # ――― Platform Specifics ――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  If this Pod runs only on iOS or OS X, then specify the platform and
  #  the deployment target. You can optionally include the target after the platform.
  #

  s.platform     = :ios
  s.platform     = :ios, "7.0"

  #  When using multiple platforms
  s.ios.deployment_target = "7.0"
  s.osx.deployment_target = "10.7"
  s.watchos.deployment_target = "2.0"
  s.tvos.deployment_target = '9.0'

  # ――― Source Location ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  Specify the location from where the source should be retrieved.
  #  Supports git, hg, bzr, svn and HTTP.
  #

  s.source       = { :git => "https://github.com/google/snappy.git", :tag => '1.1.7' }

  s.header_mappings_dir = '.'

  s.prepare_command = <<-END_OF_COMMAND
    cat > "snappy-stubs-public.h" <<EOF
    #ifndef THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_
    #define THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_

        #include <stdint.h>
        #include <stddef.h>=
        #include <sys/uio.h>
        
        #define SNAPPY_MAJOR 
        #define SNAPPY_MINOR 
        #define SNAPPY_PATCHLEVEL 
        #define SNAPPY_VERSION \
            ((SNAPPY_MAJOR << 16) | (SNAPPY_MINOR << 8) | SNAPPY_PATCHLEVEL)

        #include <string>

        namespace snappy {

            typedef int8_t int8;
            typedef uint8_t uint8;
            typedef int16_t int16;
            typedef uint16_t uint16;
            typedef int32_t int32;
            typedef uint32_t uint32;
            typedef int64_t int64;
            typedef uint64_t uint64;
            
            typedef std::string string;

        }  // namespace snappy

    #endif  // THIRD_PARTY_SNAPPY_OPENSOURCE_SNAPPY_STUBS_PUBLIC_H_
    EOF
    cat > "config.h" <<EOF
    ifndef THIRD_PARTY_SNAPPY_OPENSOURCE_CMAKE_CONFIG_H_
    #define THIRD_PARTY_SNAPPY_OPENSOURCE_CMAKE_CONFIG_H_

        /* Define to 1 if the compiler supports __builtin_ctz and friends. */
        #define HAVE_BUILTIN_CTZ 1

        /* Define to 1 if the compiler supports __builtin_expect. */
        #define HAVE_BUILTIN_EXPECT 1

        /* Define to 1 if you have the <byteswap.h> header file. */
        #define HAVE_BYTESWAP_H 1

        /* Define to 1 if you have a definition for mmap() in <sys/mman.h>. */
        #define HAVE_FUNC_MMAP 1

        /* Define to 1 if you have a definition for sysconf() in <unistd.h>. */
        #define HAVE_FUNC_SYSCONF 1

        /* Define to 1 to use the gflags package for command-line parsing. */
        /* #undef HAVE_GFLAGS */

        /* Define to 1 if you have Google Test. */
        /* #undef HAVE_GTEST */

        /* Define to 1 if you have the `lzo2' library (-llzo2). */
        /* #undef HAVE_LIBLZO2 */

        /* Define to 1 if you have the `z' library (-lz). */
        #define HAVE_LIBZ 1

        /* Define to 1 if you have the <stddef.h> header file. */
        #define HAVE_STDDEF_H 1

        /* Define to 1 if you have the <stdint.h> header file. */
        #define HAVE_STDINT_H 1

        /* Define to 1 if you have the <sys/endian.h> header file. */
        /* #undef HAVE_SYS_ENDIAN_H */

        /* Define to 1 if you have the <sys/mman.h> header file. */
        #define HAVE_SYS_MMAN_H 1

        /* Define to 1 if you have the <sys/resource.h> header file. */
        #define HAVE_SYS_RESOURCE_H 1

        /* Define to 1 if you have the <sys/time.h> header file. */
        #define HAVE_SYS_TIME_H 1

        /* Define to 1 if you have the <sys/uio.h> header file. */
        #define HAVE_SYS_UIO_H 1

        /* Define to 1 if you have the <unistd.h> header file. */
        #define HAVE_UNISTD_H 1

        /* Define to 1 if you have the <windows.h> header file. */
        /* #undef HAVE_WINDOWS_H */

        /* Define to 1 if your processor stores words with the most significant byte
        first (like Motorola and SPARC, unlike Intel and VAX). */
        /* #undef SNAPPY_IS_BIG_ENDIAN */

    #endif  // THIRD_PARTY_SNAPPY_OPENSOURCE_CMAKE_CONFIG_H_

    EOF
  END_OF_COMMAND
  
  # ――― Source Code ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  CocoaPods is smart about how it includes source code. For source files
  #  giving a folder will include any swift, h, m, mm, c & cpp files.
  #  For header files it will include any header in the folder.
  #  Not including the public_header_files will make all headers public.
  #
  s.requires_arc = false

  s.preserve_paths = "*.{h,cc}"

  s.public_header_files = '*.h'
  s.source_files = '*.{cc,h}'
  s.exclude_files = '*test.cc'
  
  s.xcconfig = { 'HEADER_SEARCH_PATHS' => '"$(PODS_ROOT)/snappy" "$(PODS_ROOT)"',  'USER_HEADER_SEARCH_PATHS' => '"$(PODS_ROOT)/snappy" "$(PODS_ROOT)"'}

  # ――― Resources ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  A list of resources included with the Pod. These are copied into the
  #  target bundle with a build phase script. Anything else will be cleaned.
  #  You can preserve files from being cleaned, please don't preserve
  #  non-essential files like tests, examples and documentation.
  #

  # s.resource  = "icon.png"
  # s.resources = "Resources/*.png"

  
  # ――― Project Linking ―――――――――――――――――――――――――――――――――――――――――――――――――――――――――― #
  #
  #  Link your library with frameworks, or libraries. Libraries do not include
  #  the lib prefix of their name.
  #
  s.libraries = "c++"

end
