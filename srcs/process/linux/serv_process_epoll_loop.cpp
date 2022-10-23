/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   serv_process_epoll_loop.cpp                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: amahla <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/10/18 12:45:35 by amahla            #+#    #+#             */
/*   Updated: 2022/10/23 21:18:46 by amahla           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include <iostream>
# include <cstring>
# include <netinet/in.h>
# include <errno.h>
# include <signal.h>
# include "webserv.h"


void	waitRequest( t_epoll & epollVar )
{
	int	timeout = 400;

	if (( epollVar.maxNbFd = epoll_wait( epollVar.epollFd, epollVar.events, 5000, timeout) ) < 0 )
		throw WebServException( "serv_process_epoll_process.cpp", "waitRequest", "select() failed" );
}

void	readData( std::vector<Client> & clients, itClient it, t_epoll & epollVar, int i )
{
		char	buffer_read[1024];
		int		rd;

		if ( (rd = recv( epollVar.events[i].data.fd , buffer_read, 1023, 0 )) <= 0 )
		{
			if ( rd < 0 )
				std::cout << RED << "Connexion client lost" << SET << std::endl;
			else
				std::cout << RED << "Connexion client is closed" << SET << std::endl;
			clients.erase( it );
			epoll_ctl(epollVar.epollFd, EPOLL_CTL_DEL, epollVar.events[i].data.fd, NULL);
			close( epollVar.events[i].data.fd );
		}
		else
		{
			buffer_read[rd] = '\0';
			// (*it).getRequest().getStringRequest() += buffer_read; For concatenate recv
			(*it).getRequest().getStringRequest() = buffer_read;

			epollVar.new_event.events = EPOLLOUT;
			epollVar.new_event.data.fd = epollVar.events[i].data.fd;
			epoll_ctl( epollVar.epollFd, EPOLL_CTL_MOD, epollVar.events[i].data.fd, &epollVar.new_event);

			std::cout << GREEN << "Server side receive from client : " << (*it).getRequest().getStringRequest() << SET << std::endl;
		}
}	

void	sendData( std::vector<Client> & clients, itClient it, t_epoll & epollVar, int i )
{
		const char	*buffer_write = (*it).getResponse().getStringResponse().c_str();

		if ( send( epollVar.events[i].data.fd, buffer_write, strlen(buffer_write), 0 ) < 0 )
		{
			std::cout << RED << "Connexion client lost" << SET << std::endl;
			clients.erase( it );
			epoll_ctl(epollVar.epollFd, EPOLL_CTL_DEL, epollVar.events[i].data.fd, NULL);
			close( epollVar.events[i].data.fd );
		}

		epollVar.new_event.events = EPOLLIN;
		epollVar.new_event.data.fd = epollVar.events[i].data.fd;
		epoll_ctl(epollVar.epollFd, EPOLL_CTL_MOD, epollVar.events[i].data.fd, &epollVar.new_event);
}

bool	ioData( std::vector<Client> & clients, t_epoll & epollVar, int i)
{
	itClient	it = find( clients, epollVar.events[i].data.fd );

	if ( epollVar.events[i].events & EPOLLIN )
	{
		readData( clients, it, epollVar, i );
		return ( true );
	}
	else if ( epollVar.events[i].data.fd & EPOLLOUT )
	{
		sendData( clients, it, epollVar, i );
		return ( true );
	}
	return ( false );
}	

void	addConnection( std::vector<Client> & clients, int server_fd, t_epoll & epollVar )
{
	struct sockaddr_in	temp;
	int					addrlen = sizeof(temp);
	int					newConnection;

	memset( (char *)&temp, 0, sizeof(temp) );

	if ( ( newConnection = accept( server_fd, reinterpret_cast< struct sockaddr *>(&temp),
					reinterpret_cast< socklen_t * >(&addrlen) ) ) < 0 )
	{
		if ( errno != EWOULDBLOCK )
			throw WebServException( "serv_process_epoll_loop.cpp", "newConnection", "accept() failed" );
	}

	else if ( newConnection >= 0 )
	{
		std::cout << YELLOW << "Connection accepted" << SET << std::endl;
		nonBlockSock( newConnection );

		epollVar.new_event.data.fd = newConnection;
		epollVar.new_event.events = EPOLLIN;
		epoll_ctl(epollVar.epollFd, EPOLL_CTL_ADD, newConnection, &(epollVar.new_event));

		clients.push_back( Client( newConnection ) );
	}
}

bool	newConnection( std::vector<Client> & clients, std::vector<Server> & servers, t_epoll & epollVar, int i )
{
	int					server_fd;

	if ( epollVar.events[i].data.fd & EPOLLERR || epollVar.events[i].events & EPOLLHUP )
	{
		clients.erase( find( clients, epollVar.events[i].data.fd ) );
		close( epollVar.events[i].data.fd );
		return ( true );
	}
	if ( ( server_fd = serverReady( servers, epollVar.events[i].data.fd ) ) >= 0 )
	{
		addConnection( clients, server_fd, epollVar );
		return ( true );
	}
	return ( false );
}

void	servProcess( std::vector<Server> & servers, std::vector<Client> & clients, t_epoll & epollVar )
{
	setEpollQueue( epollVar, servers );
	while (1)
	{
		waitRequest( epollVar );
		for ( int i(0); i < epollVar.maxNbFd; i++)
		{
			if ( newConnection( clients, servers, epollVar, i ) )
				break ;
			if ( ioData( clients, epollVar, i ) )
				break ;
		}
	}
}