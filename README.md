# da-nan
Milepæl 1 

Dere skal i første milepæl programmere en webtjener med programmeringspråket C. Ta gjerne utgangspunkt i eksemplet hallotjener.
Funksjonskrav

    Tjeneren skal leverere "as-is"-filer. Det vil si at filene inneholder både http-hode og http-kropp, og sendes "som de er", uten å legge til eller trekke fra noe. Enklere blir det ikke: Tjeneren åpner fila og skriver den til til socket'en som er forbundet med klienten.

    Disse filene skal ende på .asis og skal inneholde både http-hode og http-kropp, som i eksemplet under.

    ./eksempler/index.asis

    HTTP/1.1 200 OK
    Content-Type: text/plain

    Hallo verden :-)

    Dersom en fil som ikke finnes blir forspurt, skal en korrekt http-feilmeling retureres til klienten.
    Logg til STDERR Åpne en fil (f.eks. /var/webtjener/error.log) og koble den til stderr, slik at du kan få skrevet ut feil- og avlusnings-info.

Sikkerhets-/robusthets-krav

    Programmet skal ikke binde seg til porten en gang pr. forespørsel, men kun ved programstart.
    Hver klient-forspørsel skal behandles i en egen tråd eller prosess.
    Tjeneren skal være demonisert slik at den er uavhengig av kontroll-terminal.
    Tjeneren den lytte på port 80 uten at den kjører som brukeren root.
    … ekstra "kontainer-krav" mer kommer i løpet av uke 2.

