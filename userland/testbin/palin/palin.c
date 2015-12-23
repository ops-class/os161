/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*

	Test suite.

	This program takes the palindrome below and checks if it's
	a palindrome or not. It will hopefully exhibit an interesting
	page fault pattern.

	The palindrome was taken from

	http://www.cs.brown.edu/people/nfp/palindrome.html

	This is not large enough to really stress the VM system, but
	might be useful for testing in the early stages of the VM
	assignment.
*/

/*
A man, a plan, a caret, a ban, a myriad, a sum, a lac, a liar, a hoop,
a pint, a catalpa, a gas, an oil, a bird, a yell, a vat, a caw, a pax,
a wag, a tax, a nay, a ram, a cap, a yam, a gay, a tsar, a wall, a
car, a luger, a ward, a bin, a woman, a vassal, a wolf, a tuna, a nit,
a pall, a fret, a watt, a bay, a daub, a tan, a cab, a datum, a gall,
a hat, a fag, a zap, a say, a jaw, a lay, a wet, a gallop, a tug, a
trot, a trap, a tram, a torr, a caper, a top, a tonk, a toll, a ball,
a fair, a sax, a minim, a tenor, a bass, a passer, a capital, a rut,
an amen, a ted, a cabal, a tang, a sun, an ass, a maw, a sag, a jam,
a dam, a sub, a salt, an axon, a sail, an ad, a wadi, a radian, a
room, a rood, a rip, a tad, a pariah, a revel, a reel, a reed, a pool,
a plug, a pin, a peek, a parabola, a dog, a pat, a cud, a nu, a fan,
a pal, a rum, a nod, an eta, a lag, an eel, a batik, a mug, a mot, a
nap, a maxim, a mood, a leek, a grub, a gob, a gel, a drab, a citadel,
a total, a cedar, a tap, a gag, a rat, a manor, a bar, a gal, a cola,
a pap, a yaw, a tab, a raj, a gab, a nag, a pagan, a bag, a jar, a
bat, a way, a papa, a local, a gar, a baron, a mat, a rag, a gap, a
tar, a decal, a tot, a led, a tic, a bard, a leg, a bog, a burg, a
keel, a doom, a mix, a map, an atom, a gum, a kit, a baleen, a gala,
a ten, a don, a mural, a pan, a faun, a ducat, a pagoda, a lob, a rap,
a keep, a nip, a gulp, a loop, a deer, a leer, a lever, a hair, a pad,
a tapir, a door, a moor, an aid, a raid, a wad, an alias, an ox, an
atlas, a bus, a madam, a jag, a saw, a mass, an anus, a gnat, a lab,
a cadet, an em, a natural, a tip, a caress, a pass, a baronet, a
minimax, a sari, a fall, a ballot, a knot, a pot, a rep, a carrot,
a mart, a part, a tort, a gut, a poll, a gateway, a law, a jay, a sap,
a zag, a fat, a hall, a gamut, a dab, a can, a tabu, a day, a batt,
a waterfall, a patina, a nut, a flow, a lass, a van, a mow, a nib,
a draw, a regular, a call, a war, a stay, a gam, a yap, a cam, a ray,
an ax, a tag, a wax, a paw, a cat, a valley, a drib, a lion, a saga,
a plat, a catnip, a pooh, a rail, a calamus, a dairyman, a bater,
a canal - Panama!
*/

/* The palindrome below is a quadruple concatenation of the above */

#include <stdio.h>
#include <string.h>

char palindrome[8000] =
"amanaplanacaretabanamyriadasumalacaliarahoopapintacatalpaagasanoil"
"abirdayellavatacawapaxawagataxanayaramacapayamagayatsarawalla"
"caralugerawardabinawomanavassalawolfatunaanitapallafretawattabaya"
"daubatanacabadatumagallahatafagazapasayajawalayawetagallopatuga"
"trotatrapatramatorracaperatopatonkatollaballafairasaxaminimatenora"
"bassapasseracapitalarutanamenatedacabalatangasunanassamawasaga"
"jamadamasubasaltanaxonasailanadawadiaradianaroomaroodaripatada"
"pariaharevelareelareedapoolaplugapinapeekaparabolaadogapatacudanua"
"fanapalarumanodanetaalaganeelabatikamugamotanapamaximamooda"
"leekagrubagobageladrabacitadelatotalacedaratapagagaratamanorabara"
"galacolaapapayawatabarajagabanagapaganabagajarabatawayapapaa"
"localagarabaronamataragagapataradecalatotaledaticabardalegaboga"
"burgakeeladoomamixamapanatomagumakitabaleenagalaatenadonamurala"
"panafaunaducatapagodaalobarapakeepanipagulpaloopadeeraleeralevera"
"hairapadatapiradooramooranaidaraidawadanaliasanoxanatlasabusamadam"
"ajagasawamassananusagnatalabacadetanemanaturalatipacaressapassa"
"baronetaminimaxasariafallaballotaknotapotarepacarrotamartapartatorta"
"gutapollagatewayalawajayasapazagafatahallagamutadabacanatabuaday"
"abattawaterfallapatinaanutaflowalassavanamowanibadrawaregularacalla"
"warastayagamayapacamarayanaxatagawaxapawacatavalleyadribaliona"
"sagaaplatacatnipapooharailacalamusadairymanabateracanalpanama"
"amanaplanacaretabanamyriadasumalacaliarahoopapintacatalpaagasanoil"
"abirdayellavatacawapaxawagataxanayaramacapayamagayatsarawalla"
"caralugerawardabinawomanavassalawolfatunaanitapallafretawattabaya"
"daubatanacabadatumagallahatafagazapasayajawalayawetagallopatuga"
"trotatrapatramatorracaperatopatonkatollaballafairasaxaminimatenora"
"bassapasseracapitalarutanamenatedacabalatangasunanassamawasaga"
"jamadamasubasaltanaxonasailanadawadiaradianaroomaroodaripatada"
"pariaharevelareelareedapoolaplugapinapeekaparabolaadogapatacudanua"
"fanapalarumanodanetaalaganeelabatikamugamotanapamaximamooda"
"leekagrubagobageladrabacitadelatotalacedaratapagagaratamanorabara"
"galacolaapapayawatabarajagabanagapaganabagajarabatawayapapaa"
"localagarabaronamataragagapataradecalatotaledaticabardalegaboga"
"burgakeeladoomamixamapanatomagumakitabaleenagalaatenadonamurala"
"panafaunaducatapagodaalobarapakeepanipagulpaloopadeeraleeralevera"
"hairapadatapiradooramooranaidaraidawadanaliasanoxanatlasabusamadam"
"ajagasawamassananusagnatalabacadetanemanaturalatipacaressapassa"
"baronetaminimaxasariafallaballotaknotapotarepacarrotamartapartatorta"
"gutapollagatewayalawajayasapazagafatahallagamutadabacanatabuaday"
"abattawaterfallapatinaanutaflowalassavanamowanibadrawaregularacalla"
"warastayagamayapacamarayanaxatagawaxapawacatavalleyadribaliona"
"sagaaplatacatnipapooharailacalamusadairymanabateracanalpanama"
"amanaplanacaretabanamyriadasumalacaliarahoopapintacatalpaagasanoil"
"abirdayellavatacawapaxawagataxanayaramacapayamagayatsarawalla"
"caralugerawardabinawomanavassalawolfatunaanitapallafretawattabaya"
"daubatanacabadatumagallahatafagazapasayajawalayawetagallopatuga"
"trotatrapatramatorracaperatopatonkatollaballafairasaxaminimatenora"
"bassapasseracapitalarutanamenatedacabalatangasunanassamawasaga"
"jamadamasubasaltanaxonasailanadawadiaradianaroomaroodaripatada"
"pariaharevelareelareedapoolaplugapinapeekaparabolaadogapatacudanua"
"fanapalarumanodanetaalaganeelabatikamugamotanapamaximamooda"
"leekagrubagobageladrabacitadelatotalacedaratapagagaratamanorabara"
"galacolaapapayawatabarajagabanagapaganabagajarabatawayapapaa"
"localagarabaronamataragagapataradecalatotaledaticabardalegaboga"
"burgakeeladoomamixamapanatomagumakitabaleenagalaatenadonamurala"
"panafaunaducatapagodaalobarapakeepanipagulpaloopadeeraleeralevera"
"hairapadatapiradooramooranaidaraidawadanaliasanoxanatlasabusamadam"
"ajagasawamassananusagnatalabacadetanemanaturalatipacaressapassa"
"baronetaminimaxasariafallaballotaknotapotarepacarrotamartapartatorta"
"gutapollagatewayalawajayasapazagafatahallagamutadabacanatabuaday"
"abattawaterfallapatinaanutaflowalassavanamowanibadrawaregularacalla"
"warastayagamayapacamarayanaxatagawaxapawacatavalleyadribaliona"
"sagaaplatacatnipapooharailacalamusadairymanabateracanalpanama"
"amanaplanacaretabanamyriadasumalacaliarahoopapintacatalpaagasanoil"
"abirdayellavatacawapaxawagataxanayaramacapayamagayatsarawalla"
"caralugerawardabinawomanavassalawolfatunaanitapallafretawattabaya"
"daubatanacabadatumagallahatafagazapasayajawalayawetagallopatuga"
"trotatrapatramatorracaperatopatonkatollaballafairasaxaminimatenora"
"bassapasseracapitalarutanamenatedacabalatangasunanassamawasaga"
"jamadamasubasaltanaxonasailanadawadiaradianaroomaroodaripatada"
"pariaharevelareelareedapoolaplugapinapeekaparabolaadogapatacudanua"
"fanapalarumanodanetaalaganeelabatikamugamotanapamaximamooda"
"leekagrubagobageladrabacitadelatotalacedaratapagagaratamanorabara"
"galacolaapapayawatabarajagabanagapaganabagajarabatawayapapaa"
"localagarabaronamataragagapataradecalatotaledaticabardalegaboga"
"burgakeeladoomamixamapanatomagumakitabaleenagalaatenadonamurala"
"panafaunaducatapagodaalobarapakeepanipagulpaloopadeeraleeralevera"
"hairapadatapiradooramooranaidaraidawadanaliasanoxanatlasabusamadam"
"ajagasawamassananusagnatalabacadetanemanaturalatipacaressapassa"
"baronetaminimaxasariafallaballotaknotapotarepacarrotamartapartatorta"
"gutapollagatewayalawajayasapazagafatahallagamutadabacanatabuaday"
"abattawaterfallapatinaanutaflowalassavanamowanibadrawaregularacalla"
"warastayagamayapacamarayanaxatagawaxapawacatavalleyadribaliona"
"sagaaplatacatnipapooharailacalamusadairymanabateracanalpanama";

int
main(void)
{
	char *start, *end;

	printf("Welcome to the palindrome tester!\n");
	printf("I will take a large palindrome and test it.\n");
	printf("Here it is:\n");
	printf("%s\n", palindrome);

	printf("Testing...");
	/* skip to end */
	end = palindrome+strlen(palindrome);
	end--;

	for (start = palindrome; start <= end; start++, end--) {
		putchar('.');
		if (*start != *end) {
			printf("NOT a palindrome\n");
			return 0;
		}
	}

	printf("IS a palindrome\n");
	return 0;
}
