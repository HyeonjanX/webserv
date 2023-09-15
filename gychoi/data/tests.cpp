int	main(void)
{
	std::string	header;
	std::string	body;
	std::string	response;

//	body += "20\r\n";
//	body += "--xyz:abc:foobar\r\n";
//	body += "Content-Dispos\r\n";
//	body += "20\r\n";
//	body += "ition: form-data; name=\"field1\"\r\r\n";
//	body += "21\r\n";
//	body += "\n\r\nBonjour.\r\n";
//	body += "--xyz:abc:foobar--\r\n\r\n";
//	body += "1\r\n";
//	body += "0\r\n";
	body += "0\r\n";
	body += "AAA\r\n";

//	body = "----------------------------216945188184884858289989\r\n";
//	body += "Content-Disposition: form-data; name=\"filename\"; filename=\"cat.jpg\"\r\n";
//	body += "Content-Type: image/jpg\r\n";
//	body += "<cat.jpg> binary\r\n";
//	body += "----------------------------216945188184884858289989\r\n";
//	body += "Content-Disposition: form-data; name=\"filename\"; filename=\"dog.jpg\"\r\n";
//	body += "Content-Type: image/jpg\r\n";
//	body += "<dog.jpg> binary\r\n";
//	body += "----------------------------216945188184884858289989\r\n";
//	body += "Content-Disposition: form-data; name=\"text\"\r\n";
//	body += "\r\n";
//	body += "----------------------------216945188184884858289989--\r\n";

	header += "GET /example HTTP/1.1\r\n";
//	header += "Content-Type: text/html; charset=utf-8\r\n";
	header += "Transfer-Encoding: chunked\r\n";
//	header += "Content-Type: multipart/form-data; boundary=--------------------------216945188184884858289989\r\n";
	header += "Content-Length: " + std::to_string(body.size()) + "\r\n";
	header += "\r\n";
	response = header + body;

	Request	req(response);
	std::cout << "<======raw header======>" << std::endl;
	std::cout << req.getHttpHeader() << std::endl;

	std::cout << "<======raw body======>" << std::endl;
	std::cout << req.getHttpBody() << std::endl;

	std::cout << "<======raw content length======>" << std::endl;
	std::cout << req.getContentLength() << std::endl;

	std::cout << "<======transfer encoding======>" << std::endl;
	std::cout << req.getTransferEncoding() << std::endl;

	std::cout << "<======content type======>" << std::endl;
	std::cout << req.getContentType() << std::endl;

	std::cout << "<======request line======>" << std::endl;
	std::cout << req.getHttpMethod() << ", " << req.getRequestUrl() << ", "
		<< req.getHttpVersion() << std::endl;
	std::cout << "<=======chunked======>" << std::endl;

	if (req.getTransferEncoding() == "chunked")
	{
		std::cout << req.isLastChunk() << std::endl;
		req.handleChunkedBody();
	}
	std::cout << "<=======muitlpart======>" << std::endl;
	if (req.getContentType().find("multipart/form-data") != std::string::npos)
	{
		req.handleMultipartBody();
		std::vector<Content> contents = req.getContents();
		for (std::vector<Content>::iterator it = contents.begin();
			it != contents.end(); ++it)
			std::cout << it->name << ", " << it->filename << ", "
			<< it->type << ", " << it->data << std::endl;
	}

	std::cout << std::endl;
	std::cout << "<======FINAL BODY======>" << std::endl;
	std::cout << req.getHttpBody() << std::endl;

	std::cout << "<======FINAL HEADER======>" << std::endl;
	std::vector<Header>	headers = req.getHttpHeaders();
	for (std::vector<Header>::iterator it = headers.begin();
		it != headers.end(); ++it)
		std::cout << it->key << ": " << it->value << std::endl;

	return 0;
}
