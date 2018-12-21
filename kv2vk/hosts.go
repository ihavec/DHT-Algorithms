package main

/*
[input]
./hosts
102.168.1.1 domain1 domain2
192.168.1.2 domain1

[output]
map[102.168.1.1:[domain1 domain2]]
map[domain1:[102.168.1.1] domain2:[102.168.1.1]]
*/
import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

/* 1. hosts -> m[ip]=domains */
func readhosts(filename string) map[string][]string {
	var fp *os.File
	var err error
	var line string
	var reader *bufio.Reader

	fp, err = os.Open(filename)
	if err != nil {
		return nil
	}
	defer fp.Close()

	m := make(map[string][]string)
	reader = bufio.NewReader(fp)

	line, err = reader.ReadString('\n')
	for err == nil {

		// trim line
		if sep := strings.IndexByte(line, '#'); sep >= 0 {
			line = line[0:sep] // discard comments.
		}
		trim := strings.TrimRight(line, " \r\n")

		// hosts -> m[ip]=domains
		split := strings.Split(trim, " ") //slice
		len := len(split)
		if len > 1 {
			ip := split[0]
			domains := []string{} //slice
			for i := 1; i < len; i++ {
				domains = append(domains, split[i])
			}
			m[ip] = domains
		}
		line, err = reader.ReadString('\n')
	}

	return m
}

/* m[ip]=domains -> m[domain]=ips */
func kv2vk(mip map[string][]string) map[string][]string {
	m := make(map[string][]string)
	for key, values := range mip {
		for _, value := range values {
			m[value] = append(m[value], key)
		}
	}
	return m
}

func main() {
	// hosts -> mip[ip]=domains
	mip := readhosts("./hosts")
	fmt.Println(mip)

	//mip[ip]=domains -> mdo[domain]=ips
	mdo := kv2vk(mip)
	fmt.Println(mdo)
}
