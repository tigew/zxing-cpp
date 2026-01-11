Pod::Spec.new do |s|
  s.name = 'zxing-cpp'
  s.version = '2.3.0'
  s.summary = 'C++ port of ZXing'
  s.homepage = 'https://github.com/zxing-cpp/zxing-cpp'
  s.author = 'axxel'
  s.readme = 'https://raw.githubusercontent.com/zxing-cpp/zxing-cpp/master/wrappers/ios/README.md'
  s.license = {
    :type => 'Apache License 2.0',
    :file => 'LICENSE'
  }
  s.source = {
    :git => 'https://github.com/zxing-cpp/zxing-cpp.git',
    :tag => "v#{s.version}"
  }
  s.module_name = 'ZXingCpp'
  s.platform = :ios, '11.0'
  s.library = ['c++']
  s.compiler_flags = [
    '-DZXING_READERS',
    '-DZXING_WITH_1D',
    '-DZXING_WITH_QRCODE',
    '-DZXING_WITH_AZTEC',
    '-DZXING_WITH_AZTECRUNE',
    '-DZXING_WITH_CODEONE',
    '-DZXING_WITH_DATAMATRIX',
    '-DZXING_WITH_DOTCODE',
    '-DZXING_WITH_GRIDMATRIX',
    '-DZXING_WITH_HANXIN',
    '-DZXING_WITH_MAXICODE',
    '-DZXING_WITH_PDF417',
    '-DZXING_WITH_MICROQRCODE',
    '-DZXING_WITH_RMQRCODE',
    '-DZXING_WITH_UPNQR',
    '-DZXING_WITH_EAN8',
    '-DZXING_WITH_EAN13',
    '-DZXING_WITH_UPCA',
    '-DZXING_WITH_UPCE',
    '-DZXING_WITH_CODE39',
    '-DZXING_WITH_CODE93',
    '-DZXING_WITH_CODE128',
    '-DZXING_WITH_CODABAR',
    '-DZXING_WITH_ITF',
    '-DZXING_WITH_CODE11',
    '-DZXING_WITH_MSI',
    '-DZXING_WITH_TELEPEN',
    '-DZXING_WITH_LOGMARS',
    '-DZXING_WITH_DXFILMEDGE',
    '-DZXING_WITH_CHANNELCODE',
    '-DZXING_WITH_DATABAR',
    '-DZXING_WITH_DATABAREXPANDED',
    '-DZXING_WITH_DATABARLIMITED',
    '-DZXING_WITH_DATABARSTACKED',
    '-DZXING_WITH_DATABARSTACKEDOMNIDIRECTIONAL',
    '-DZXING_WITH_DATABAREXPANDEDSTACKED',
    '-DZXING_WITH_AUSTRALIAPOST',
    '-DZXING_WITH_KIXCODE',
    '-DZXING_WITH_JAPANPOST',
    '-DZXING_WITH_KOREAPOST',
    '-DZXING_WITH_RM4SCC',
    '-DZXING_WITH_MAILMARK',
    '-DZXING_WITH_USPSIMB',
    '-DZXING_WITH_DEUTSCHEPOSTLEITCODE',
    '-DZXING_WITH_DEUTSCHEPOSTIDENTCODE',
    '-DZXING_WITH_POSTNET',
    '-DZXING_WITH_PLANET',
    '-DZXING_WITH_CODE32',
    '-DZXING_WITH_PHARMACODE',
    '-DZXING_WITH_PHARMACODETWOTRACK',
    '-DZXING_WITH_PZN',
    '-DZXING_WITH_MATRIX2OF5',
    '-DZXING_WITH_INDUSTRIAL2OF5',
    '-DZXING_WITH_IATA2OF5',
    '-DZXING_WITH_DATALOGIC2OF5',
    '-DZXING_WITH_CODABLOCKF',
    '-DZXING_WITH_CODE16K',
    '-DZXING_WITH_CODE49',
    '-DZXING_ENABLE_EAN8',
    '-DZXING_ENABLE_EAN13',
    '-DZXING_ENABLE_UPCA',
    '-DZXING_ENABLE_UPCE',
    '-DZXING_ENABLE_CODE39',
    '-DZXING_ENABLE_CODE93',
    '-DZXING_ENABLE_CODE128',
    '-DZXING_ENABLE_CODABAR',
    '-DZXING_ENABLE_ITF',
    '-DZXING_ENABLE_CODE11',
    '-DZXING_ENABLE_MSI',
    '-DZXING_ENABLE_TELEPEN',
    '-DZXING_ENABLE_LOGMARS',
    '-DZXING_ENABLE_DXFILMEDGE',
    '-DZXING_ENABLE_CHANNELCODE',
    '-DZXING_ENABLE_DATABAR',
    '-DZXING_ENABLE_DATABAREXPANDED',
    '-DZXING_ENABLE_DATABARLIMITED',
    '-DZXING_ENABLE_DATABARSTACKED',
    '-DZXING_ENABLE_DATABARSTACKEDOMNIDIRECTIONAL',
    '-DZXING_ENABLE_DATABAREXPANDEDSTACKED',
    '-DZXING_ENABLE_AUSTRALIAPOST',
    '-DZXING_ENABLE_KIXCODE',
    '-DZXING_ENABLE_JAPANPOST',
    '-DZXING_ENABLE_KOREAPOST',
    '-DZXING_ENABLE_RM4SCC',
    '-DZXING_ENABLE_MAILMARK',
    '-DZXING_ENABLE_USPSIMB',
    '-DZXING_ENABLE_DEUTSCHEPOSTLEITCODE',
    '-DZXING_ENABLE_DEUTSCHEPOSTIDENTCODE',
    '-DZXING_ENABLE_POSTNET',
    '-DZXING_ENABLE_PLANET',
    '-DZXING_ENABLE_CODE32',
    '-DZXING_ENABLE_PHARMACODE',
    '-DZXING_ENABLE_PHARMACODETWOTRACK',
    '-DZXING_ENABLE_PZN',
    '-DZXING_ENABLE_MATRIX2OF5',
    '-DZXING_ENABLE_INDUSTRIAL2OF5',
    '-DZXING_ENABLE_IATA2OF5',
    '-DZXING_ENABLE_DATALOGIC2OF5',
    '-DZXING_ENABLE_CODABLOCKF',
    '-DZXING_ENABLE_CODE16K',
    '-DZXING_ENABLE_CODE49',
    '-DZXING_ENABLE_QRCODE',
    '-DZXING_ENABLE_MICROQRCODE',
    '-DZXING_ENABLE_RMQRCODE',
    '-DZXING_ENABLE_UPNQR',
    '-DZXING_ENABLE_AZTEC',
    '-DZXING_ENABLE_AZTECRUNE',
    '-DZXING_ENABLE_CODEONE',
    '-DZXING_ENABLE_DATAMATRIX',
    '-DZXING_ENABLE_DOTCODE',
    '-DZXING_ENABLE_GRIDMATRIX',
    '-DZXING_ENABLE_HANXIN',
    '-DZXING_ENABLE_MAXICODE',
    '-DZXING_ENABLE_PDF417'
  ]
  s.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++20'
  }

  s.default_subspec = 'Wrapper'

  s.subspec 'Core' do |ss|
    ss.source_files = 'core/src/**/*.{h,c,cpp}'
    ss.exclude_files = [ 'core/src/libzint/**' ]
    ss.private_header_files = 'core/src/**/*.h'
  end

  s.subspec 'Wrapper' do |ss|
    ss.dependency 'zxing-cpp/Core'
    ss.frameworks = 'CoreGraphics', 'CoreImage', 'CoreVideo'
    ss.source_files = 'wrappers/ios/Sources/Wrapper/**/*.{h,m,mm}'
    ss.public_header_files = 'wrappers/ios/Sources/Wrapper/Reader/{ZXIBarcodeReader,ZXIResult,ZXIPosition,ZXIPoint,ZXIGTIN,ZXIReaderOptions}.h',
                             'wrappers/ios/Sources/Wrapper/Writer/{ZXIBarcodeWriter,ZXIWriterOptions}.h',
                             'wrappers/ios/Sources/Wrapper/{ZXIErrors,ZXIFormat}.h'
    ss.exclude_files = 'wrappers/ios/Sources/Wrapper/UmbrellaHeader.h'
  end
end
