# -*- encoding: utf-8 -*-
# frozen_string_literal: false
describe :string_encode, shared: true do
  describe "when passed no options" do
    it "transcodes to Encoding.default_internal when set" do
      Encoding.default_internal = Encoding::UTF_8
      str = [0xA4, 0xA2].pack('CC').force_encoding Encoding::EUC_JP
      str.send(@method).should == "あ"
    end

    it "transcodes a 7-bit String despite no generic converting being available" do
      NATFIXME 'need Emacs_Mule encoding', exception: SpecFailedException, message: /uninitialized constant Encoding::Emacs_Mule/ do
        -> do
          Encoding::Converter.new Encoding::Emacs_Mule, Encoding::BINARY
        end.should raise_error(Encoding::ConverterNotFoundError)

        Encoding.default_internal = Encoding::Emacs_Mule
        str = "\x79".force_encoding Encoding::BINARY

        str.send(@method).should == "y".force_encoding(Encoding::BINARY)
      end
    end

    it "raises an Encoding::ConverterNotFoundError when no conversion is possible" do
      NATFIXME 'need Emacs_Mule encoding', exception: NameError, message: /uninitialized constant Encoding::Emacs_Mule/ do
        Encoding.default_internal = Encoding::Emacs_Mule
        str = [0x80].pack('C').force_encoding Encoding::BINARY
        -> { str.send(@method) }.should raise_error(Encoding::ConverterNotFoundError)
      end
    end
  end

  describe "when passed to encoding" do
    it "accepts a String argument" do
      str = [0xA4, 0xA2].pack('CC').force_encoding Encoding::EUC_JP
      str.send(@method, "utf-8").should == "あ"
    end

    it "calls #to_str to convert the object to an Encoding" do
      enc = mock("string encode encoding")
      enc.should_receive(:to_str).and_return("utf-8")

      str = [0xA4, 0xA2].pack('CC').force_encoding Encoding::EUC_JP
      str.send(@method, enc).should == "あ"
    end

    it "transcodes to the passed encoding" do
      str = [0xA4, 0xA2].pack('CC').force_encoding Encoding::EUC_JP
      str.send(@method, Encoding::UTF_8).should == "あ"
    end

    it "transcodes Japanese multibyte characters" do
      NATFIXME 'need ISO_2022_JP encoding', exception: NameError, message: /uninitialized constant Encoding::ISO_2022_JP/ do
        str = "あいうえお"
        str.send(@method, Encoding::ISO_2022_JP).should ==
          "\e\x24\x42\x24\x22\x24\x24\x24\x26\x24\x28\x24\x2A\e\x28\x42".force_encoding(Encoding::ISO_2022_JP)
      end
    end

    it "transcodes a 7-bit String despite no generic converting being available" do
      NATFIXME 'need Emacs_Mule encoding', exception: SpecFailedException, message: /uninitialized constant Encoding::Emacs_Mule/ do
        -> do
          Encoding::Converter.new Encoding::Emacs_Mule, Encoding::BINARY
        end.should raise_error(Encoding::ConverterNotFoundError)

        str = "\x79".force_encoding Encoding::BINARY
        str.send(@method, Encoding::Emacs_Mule).should == "y".force_encoding(Encoding::BINARY)
      end
    end

    it "raises an Encoding::ConverterNotFoundError when no conversion is possible" do
      NATFIXME 'need Emacs_Mule encoding', exception: SpecFailedException, message: /uninitialized constant Encoding::Emacs_Mule/ do
        str = [0x80].pack('C').force_encoding Encoding::BINARY
        -> do
          str.send(@method, Encoding::Emacs_Mule)
        end.should raise_error(Encoding::ConverterNotFoundError)
      end
    end

    it "raises an Encoding::ConverterNotFoundError for an invalid encoding" do
      NATFIXME 'wrong error', exception: SpecFailedException, message: /ConverterNotFoundError.*but instead.*ArgumentError/ do
        -> do
          "abc".send(@method, "xyz")
        end.should raise_error(Encoding::ConverterNotFoundError)
      end
    end
  end

  describe "when passed options" do
    it "does not process transcoding options if not transcoding" do
      result = "あ\ufffdあ".send(@method, undef: :replace)
      result.should == "あ\ufffdあ"
    end

    it "calls #to_hash to convert the object" do
      options = mock("string encode options")
      options.should_receive(:to_hash).and_return({ undef: :replace })

      result = "あ\ufffdあ".send(@method, **options)
      result.should == "あ\ufffdあ"
    end

    it "transcodes to Encoding.default_internal when set" do
      Encoding.default_internal = Encoding::UTF_8
      str = [0xA4, 0xA2].pack('CC').force_encoding Encoding::EUC_JP
      str.send(@method, invalid: :replace).should == "あ"
    end

    it "raises an Encoding::ConverterNotFoundError when no conversion is possible despite 'invalid: :replace, undef: :replace'" do
      NATFIXME 'need Emacs_Mule encoding', exception: NameError, message: /uninitialized constant Encoding::Emacs_Mule/ do
        Encoding.default_internal = Encoding::Emacs_Mule
        str = [0x80].pack('C').force_encoding Encoding::BINARY
        -> do
          str.send(@method, invalid: :replace, undef: :replace)
        end.should raise_error(Encoding::ConverterNotFoundError)
      end
    end

    it "replaces invalid characters when replacing Emacs-Mule encoded strings" do
      NATFIXME 'need Emacs_Mule encoding', exception: ArgumentError, message: 'unknown encoding name - "Emacs-Mule"' do
        got = [0x80].pack('C').force_encoding('Emacs-Mule').send(@method, invalid: :replace)

        got.should == "?".encode('Emacs-Mule')
      end
    end
  end

  describe "when passed to, from" do
    it "transcodes between the encodings ignoring the String encoding" do
      NATFIXME 'honor source encoding', exception: SpecFailedException, message: /should be ==/ do
        str = "あ"
        result = [0xA6, 0xD0, 0x8F, 0xAB, 0xE4, 0x8F, 0xAB, 0xB1].pack('C8')
        result.force_encoding Encoding::EUC_JP
        str.send(@method, "euc-jp", "ibm437").should == result
      end
    end

    it "calls #to_str to convert the from object to an Encoding" do
      NATFIXME 'honor source encoding', exception: SpecFailedException, message: /should be ==/ do
        enc = mock("string encode encoding")
        enc.should_receive(:to_str).and_return("ibm437")

        str = "あ"
        result = [0xA6, 0xD0, 0x8F, 0xAB, 0xE4, 0x8F, 0xAB, 0xB1].pack('C8')
        result.force_encoding Encoding::EUC_JP

        str.send(@method, "euc-jp", enc).should == result
      end
    end
  end

  describe "when passed to, options" do
    it "replaces undefined characters in the destination encoding" do
      result = "あ?あ".send(@method, Encoding::EUC_JP, undef: :replace)
      # testing for: "\xA4\xA2?\xA4\xA2"
      xA4xA2 = [0xA4, 0xA2].pack('CC')
      result.should == "#{xA4xA2}?#{xA4xA2}".force_encoding("euc-jp")
    end

    it "replaces invalid characters in the destination encoding" do
      xFF = [0xFF].pack('C').force_encoding('utf-8')
      "ab#{xFF}c".send(@method, Encoding::ISO_8859_1, invalid: :replace).should == "ab?c"
    end

    it "calls #to_hash to convert the options object" do
      options = mock("string encode options")
      options.should_receive(:to_hash).and_return({ undef: :replace })

      result = "あ?あ".send(@method, Encoding::EUC_JP, **options)
      xA4xA2 = [0xA4, 0xA2].pack('CC').force_encoding('utf-8')
      result.should == "#{xA4xA2}?#{xA4xA2}".force_encoding("euc-jp")
    end
  end

  describe "when passed to, from, options" do
    it "replaces undefined characters in the destination encoding" do
      NATFIXME 'honor source encoding', exception: Encoding::UndefinedConversionError, message: /to UTF-8 in conversion from ASCII-8BIT to UTF-8 to EUC-JP/ do
        str = "あ?あ".force_encoding Encoding::BINARY
        result = str.send(@method, "euc-jp", "utf-8", undef: :replace)
        xA4xA2 = [0xA4, 0xA2].pack('CC').force_encoding('utf-8')
        result.should == "#{xA4xA2}?#{xA4xA2}".force_encoding("euc-jp")
      end
    end

    it "replaces invalid characters in the destination encoding" do
      NATFIXME 'honor source encoding', exception: Encoding::UndefinedConversionError, message: /to UTF-8 in conversion from ASCII-8BIT to UTF-8 to ISO-8859-1/ do
        xFF = [0xFF].pack('C').force_encoding('utf-8')
        str = "ab#{xFF}c".force_encoding Encoding::BINARY
        str.send(@method, "iso-8859-1", "utf-8", invalid: :replace).should == "ab?c"
      end
    end

    it "calls #to_str to convert the to object to an encoding" do
      NATFIXME 'honor source encoding', exception: Encoding::UndefinedConversionError, message: /to UTF-8 in conversion from ASCII-8BIT to UTF-8 to ISO-8859-1/ do
        to = mock("string encode to encoding")
        to.should_receive(:to_str).and_return("iso-8859-1")

        xFF = [0xFF].pack('C').force_encoding('utf-8')
        str = "ab#{xFF}c".force_encoding Encoding::BINARY
        str.send(@method, to, "utf-8", invalid: :replace).should == "ab?c"
      end
    end

    it "calls #to_str to convert the from object to an encoding" do
      NATFIXME 'honor source encoding', exception: Encoding::UndefinedConversionError, message: /to UTF-8 in conversion from ASCII-8BIT to UTF-8 to ISO-8859-1/ do
        from = mock("string encode to encoding")
        from.should_receive(:to_str).and_return("utf-8")

        xFF = [0xFF].pack('C').force_encoding('utf-8')
        str = "ab#{xFF}c".force_encoding Encoding::BINARY
        str.send(@method, "iso-8859-1", from, invalid: :replace).should == "ab?c"
      end
    end

    it "calls #to_hash to convert the options object" do
      NATFIXME 'keyword splat should call to_hash?' do
        options = mock("string encode options")
        options.should_receive(:to_hash).and_return({ invalid: :replace })

        xFF = [0xFF].pack('C').force_encoding('utf-8')
        str = "ab#{xFF}c".force_encoding Encoding::BINARY
        str.send(@method, "iso-8859-1", "utf-8", **options).should == "ab?c"
      end
    end
  end

  describe "given the xml: :text option" do
    it "replaces all instances of '&' with '&amp;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '& and &'.send(@method, "UTF-8", xml: :text).should == '&amp; and &amp;'
      end
    end

    it "replaces all instances of '<' with '&lt;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '< and <'.send(@method, "UTF-8", xml: :text).should == '&lt; and &lt;'
      end
    end

    it "replaces all instances of '>' with '&gt;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '> and >'.send(@method, "UTF-8", xml: :text).should == '&gt; and &gt;'
      end
    end

    it "does not replace '\"'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '" and "'.send(@method, "UTF-8", xml: :text).should == '" and "'
      end
    end

    it "replaces undefined characters with their upper-case hexadecimal numeric character references" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        'ürst'.send(@method, Encoding::US_ASCII, xml: :text).should == '&#xFC;rst'
      end
    end
  end

  describe "given the xml: :attr option" do
    it "surrounds the encoded text with double-quotes" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        'abc'.send(@method, "UTF-8", xml: :attr).should == '"abc"'
      end
    end

    it "replaces all instances of '&' with '&amp;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '& and &'.send(@method, "UTF-8", xml: :attr).should == '"&amp; and &amp;"'
      end
    end

    it "replaces all instances of '<' with '&lt;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '< and <'.send(@method, "UTF-8", xml: :attr).should == '"&lt; and &lt;"'
      end
    end

    it "replaces all instances of '>' with '&gt;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '> and >'.send(@method, "UTF-8", xml: :attr).should == '"&gt; and &gt;"'
      end
    end

    it "replaces all instances of '\"' with '&quot;'" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        '" and "'.send(@method, "UTF-8", xml: :attr).should == '"&quot; and &quot;"'
      end
    end

    it "replaces undefined characters with their upper-case hexadecimal numeric character references" do
      NATFIXME 'xml option', exception: ArgumentError, message: 'unknown keyword: :xml' do
        'ürst'.send(@method, Encoding::US_ASCII, xml: :attr).should == '"&#xFC;rst"'
      end
    end
  end

  it "raises ArgumentError if the value of the :xml option is not :text or :attr" do
    -> { ''.send(@method, "UTF-8", xml: :other) }.should raise_error(ArgumentError)
  end
end
