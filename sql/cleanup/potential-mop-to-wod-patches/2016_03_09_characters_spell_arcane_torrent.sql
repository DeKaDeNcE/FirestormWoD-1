DELETE FROM character_spell WHERE `spell`=28730 AND `guid` IN(SELECT `guid` FROM characters WHERE `class`=2 AND `race`=10);