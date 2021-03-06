
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2013 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_phalcon.h"
#include "phalcon.h"

#include "Zend/zend_operators.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"

#include "kernel/main.h"
#include "kernel/memory.h"

#include "kernel/exception.h"
#include "kernel/object.h"
#include "kernel/fcall.h"
#include "kernel/filter.h"

/**
 * Phalcon\Escaper
 *
 * Escapes different kinds of text securing them. By using this component you may
 * prevent XSS attacks.
 *
 * This component only works with UTF-8. The PREG extension needs to be compiled with UTF-8 support.
 *
 *<code>
 *	$escaper = new Phalcon\Escaper();
 *	$escaped = $escaper->escapeCss("font-family: <Verdana>");
 *	echo $escaped; // font\2D family\3A \20 \3C Verdana\3E
 *</code>
 */


/**
 * Phalcon\Escaper initializer
 */
PHALCON_INIT_CLASS(Phalcon_Escaper){

	PHALCON_REGISTER_CLASS(Phalcon, Escaper, escaper, phalcon_escaper_method_entry, 0);

	zend_declare_property_string(phalcon_escaper_ce, SL("_encoding"), "utf-8", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(phalcon_escaper_ce, SL("_htmlEscapeMap"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(phalcon_escaper_ce, SL("_htmlQuoteType"), 3, ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_class_implements(phalcon_escaper_ce TSRMLS_CC, 1, phalcon_escaperinterface_ce);

	return SUCCESS;
}

/**
 * Sets the encoding to be used by the escaper
 *
 *<code>
 * $escaper->setEncoding('utf-8');
 *</code>
 *
 * @param string $encoding
 */
PHP_METHOD(Phalcon_Escaper, setEncoding){

	zval *encoding;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &encoding) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(encoding) != IS_STRING) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_escaper_exception_ce, "The character set must be string");
		return;
	}
	phalcon_update_property_zval(this_ptr, SL("_encoding"), encoding TSRMLS_CC);
	
	PHALCON_MM_RESTORE();
}

/**
 * Returns the internal encoding used by the escaper
 *
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, getEncoding){


	RETURN_MEMBER(this_ptr, "_encoding");
}

/**
 * Sets the HTML quoting type for htmlspecialchars
 *
 *<code>
 * $escaper->setHtmlQuoteType(ENT_XHTML);
 *</code>
 *
 * @param int $quoteType
 */
PHP_METHOD(Phalcon_Escaper, setHtmlQuoteType){

	zval *quote_type;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &quote_type) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(quote_type) != IS_LONG) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_escaper_exception_ce, "The quoting type is not valid");
		return;
	}
	phalcon_update_property_zval(this_ptr, SL("_htmlQuoteType"), quote_type TSRMLS_CC);
	
	PHALCON_MM_RESTORE();
}

/**
 * Detect the character encoding of a string to be handled by an encoder
 * Special-handling for chr(172) and chr(128) to chr(159) which fail to be detected by mb_detect_encoding()
 *
 * @param string $str
 * @param string $charset
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, detectEncoding){

	zval *str, *charset = NULL, *strict_check, *detected = NULL;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &str) == FAILURE) {
		RETURN_MM_NULL();
	}

	/** 
	 * Check if charset is ASCII or ISO-8859-1
	 */
	PHALCON_INIT_VAR(charset);
	phalcon_is_basic_charset(charset, str);
	if (Z_TYPE_P(charset) == IS_STRING) {
		RETURN_CTOR(charset);
	}
	
	/** 
	 * We require mbstring extension here
	 */
	if (phalcon_function_exists_ex(SS("mb_detect_encoding") TSRMLS_CC) == FAILURE) {
		RETURN_MM_NULL();
	}
	
	/** 
	 * Strict encoding detection with fallback to non-strict detection.
	 */
	PHALCON_INIT_VAR(strict_check);
	ZVAL_BOOL(strict_check, 1);
	
	PHALCON_INIT_NVAR(charset);
	ZVAL_STRING(charset, "UTF-32", 1);
	
	/** 
	 * Check for UTF-32 encoding
	 */
	PHALCON_INIT_VAR(detected);
	PHALCON_CALL_FUNC_PARAMS_3(detected, "mb_detect_encoding", str, charset, strict_check);
	if (zend_is_true(detected)) {
		RETURN_CTOR(charset);
	}
	
	PHALCON_INIT_NVAR(charset);
	ZVAL_STRING(charset, "UTF-16", 1);
	
	/** 
	 * Check for UTF-16 encoding
	 */
	PHALCON_INIT_NVAR(detected);
	PHALCON_CALL_FUNC_PARAMS_3(detected, "mb_detect_encoding", str, charset, strict_check);
	if (zend_is_true(detected)) {
		RETURN_CTOR(charset);
	}
	
	PHALCON_INIT_NVAR(charset);
	ZVAL_STRING(charset, "UTF-8", 1);
	
	/** 
	 * Check for UTF-8 encoding
	 */
	PHALCON_INIT_NVAR(detected);
	PHALCON_CALL_FUNC_PARAMS_3(detected, "mb_detect_encoding", str, charset, strict_check);
	if (zend_is_true(detected)) {
		RETURN_CTOR(charset);
	}
	
	PHALCON_INIT_NVAR(charset);
	ZVAL_STRING(charset, "ISO-8859-1", 1);
	
	/** 
	 * Check for ISO-8859-1 encoding
	 */
	PHALCON_INIT_NVAR(detected);
	PHALCON_CALL_FUNC_PARAMS_3(detected, "mb_detect_encoding", str, charset, strict_check);
	if (zend_is_true(detected)) {
		RETURN_CTOR(charset);
	}
	
	PHALCON_INIT_NVAR(charset);
	ZVAL_STRING(charset, "ASCII", 1);
	
	/** 
	 * Check for ASCII encoding
	 */
	PHALCON_INIT_NVAR(detected);
	PHALCON_CALL_FUNC_PARAMS_3(detected, "mb_detect_encoding", str, charset, strict_check);
	if (zend_is_true(detected)) {
		RETURN_CTOR(charset);
	}
	
	/** 
	 * Fallback to global detection
	 */
	PHALCON_INIT_NVAR(charset);
	PHALCON_CALL_FUNC_PARAMS_1(charset, "mb_detect_encoding", str);
	
	RETURN_CCTOR(charset);
}

/**
 * Utility to normalize a string's encoding to UTF-32.
 *
 * @param string $str
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, normalizeEncoding){

	zval *str, *encoding, *charset, *encoded;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &str) == FAILURE) {
		RETURN_MM_NULL();
	}

	/** 
	 * mbstring is required here
	 */
	if (phalcon_function_exists_ex(SS("mb_convert_encoding") TSRMLS_CC) == FAILURE) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_escaper_exception_ce, "Extension 'mbstring' is required");
		return;
	}
	
	PHALCON_INIT_VAR(encoding);
	PHALCON_CALL_METHOD_PARAMS_1(encoding, this_ptr, "detectencoding", str);
	
	PHALCON_INIT_VAR(charset);
	ZVAL_STRING(charset, "UTF-32", 1);
	
	/** 
	 * Convert to UTF-32 (4 byte characters, regardless of actual number of bytes in
	 * the character).
	 */
	PHALCON_INIT_VAR(encoded);
	PHALCON_CALL_FUNC_PARAMS_3(encoded, "mb_convert_encoding", str, charset, encoding);
	
	RETURN_CCTOR(encoded);
}

/**
 * Escapes a HTML string. Internally uses htmlspeciarchars
 *
 * @param string $text
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, escapeHtml){

	zval *text, *html_quote_type, *encoding, *escaped;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &text) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(text) == IS_STRING) {
		PHALCON_OBS_VAR(html_quote_type);
		phalcon_read_property(&html_quote_type, this_ptr, SL("_htmlQuoteType"), PH_NOISY_CC);
	
		PHALCON_OBS_VAR(encoding);
		phalcon_read_property(&encoding, this_ptr, SL("_encoding"), PH_NOISY_CC);
	
		PHALCON_INIT_VAR(escaped);
		phalcon_escape_html(escaped, text, html_quote_type, encoding TSRMLS_CC);
		RETURN_CTOR(escaped);
	}
	RETURN_MM_NULL();
}

/**
 * Escapes a HTML attribute string
 *
 * @param string $attribute
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, escapeHtmlAttr){

	zval *attribute, *normalized, *sanitized;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &attribute) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(attribute) == IS_STRING) {
		if (zend_is_true(attribute)) {
			/** 
			 * Normalize encoding to UTF-32
			 */
			PHALCON_INIT_VAR(normalized);
			PHALCON_CALL_METHOD_PARAMS_1(normalized, this_ptr, "normalizeencoding", attribute);
	
			/** 
			 * Escape the string
			 */
			PHALCON_INIT_VAR(sanitized);
			phalcon_escape_htmlattr(sanitized, normalized);
			RETURN_CTOR(sanitized);
		}
	}
	RETURN_MM_NULL();
}

/**
 * Escape CSS strings by replacing non-alphanumeric chars by their hexadecimal escaped representation
 *
 * @param string $css
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, escapeCss){

	zval *css, *normalized, *sanitized;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &css) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(css) == IS_STRING) {
		if (zend_is_true(css)) {
			/** 
			 * Normalize encoding to UTF-32
			 */
			PHALCON_INIT_VAR(normalized);
			PHALCON_CALL_METHOD_PARAMS_1(normalized, this_ptr, "normalizeencoding", css);
	
			/** 
			 * Escape the string
			 */
			PHALCON_INIT_VAR(sanitized);
			phalcon_escape_css(sanitized, normalized);
			RETURN_CTOR(sanitized);
		}
	}
	RETURN_MM_NULL();
}

/**
 * Escape javascript strings by replacing non-alphanumeric chars by their hexadecimal escaped representation
 *
 * @param string $js
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, escapeJs){

	zval *js, *normalized, *sanitized;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &js) == FAILURE) {
		RETURN_MM_NULL();
	}

	if (Z_TYPE_P(js) == IS_STRING) {
		if (zend_is_true(js)) {
			/** 
			 * Normalize encoding to UTF-32
			 */
			PHALCON_INIT_VAR(normalized);
			PHALCON_CALL_METHOD_PARAMS_1(normalized, this_ptr, "normalizeencoding", js);
	
			/** 
			 * Escape the string
			 */
			PHALCON_INIT_VAR(sanitized);
			phalcon_escape_js(sanitized, normalized);
			RETURN_CTOR(sanitized);
		}
	}
	RETURN_MM_NULL();
}

/**
 * Escapes a URL. Internally uses rawurlencode
 *
 * @param string $url
 * @return string
 */
PHP_METHOD(Phalcon_Escaper, escapeUrl){

	zval *url, *escaped;

	PHALCON_MM_GROW();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &url) == FAILURE) {
		RETURN_MM_NULL();
	}

	PHALCON_INIT_VAR(escaped);
	PHALCON_CALL_FUNC_PARAMS_1(escaped, "rawurlencode", url);
	RETURN_CCTOR(escaped);
}

