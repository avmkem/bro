#include "SSL-binpac.h"
#include "TCP_Reassembler.h"
#include "Reporter.h"
#include "util.h"

SSL_Analyzer_binpac::SSL_Analyzer_binpac(Connection* c)
: TCP_ApplicationAnalyzer(AnalyzerTag::SSL, c)
	{
	interp = new binpac::SSL::SSL_Conn(this);
	had_gap = false;
	}

SSL_Analyzer_binpac::~SSL_Analyzer_binpac()
	{
	delete interp;
	}

void SSL_Analyzer_binpac::Done()
	{
	TCP_ApplicationAnalyzer::Done();

	interp->FlowEOF(true);
	interp->FlowEOF(false);
	}

void SSL_Analyzer_binpac::EndpointEOF(TCP_Reassembler* endp)
	{
	TCP_ApplicationAnalyzer::EndpointEOF(endp);
	interp->FlowEOF(endp->IsOrig());
	}

void SSL_Analyzer_binpac::DeliverStream(int len, const u_char* data, bool orig)
	{
	TCP_ApplicationAnalyzer::DeliverStream(len, data, orig);

	assert(TCP());

	if ( TCP()->IsPartial() )
		return;
	if ( had_gap )
		// XXX: If only one side had a content gap, we could still try to
		// deliver data to the other side if the script layer can handle this.
		return; 

	try
		{
		interp->NewData(orig, data, data + len);
		}
	catch ( binpac::Exception const &e )
		{
		ProtocolViolation(fmt("Binpac exception: %s", e.c_msg()));
		}
	}

void SSL_Analyzer_binpac::Undelivered(int seq, int len, bool orig)
	{
	TCP_ApplicationAnalyzer::Undelivered(seq, len, orig);
	had_gap = true;
	interp->NewGap(orig, len);
	}
